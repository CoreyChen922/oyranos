#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sane/sane.h>

#include <oyranos_alpha.h>
#define CMM_BASE_REG "//imaging/config.scanner.SANE"
#define CMM_COMMAND CMM_BASE_REG "/command"

SANE_Status status;
SANE_Int version = 0, num_options, num_devices ;
SANE_Handle device_handle;
const SANE_Device **device_list = NULL;
SANE_Device *sane_device = NULL;
SANE_String_Const device_name = NULL;

oyOption_s *option = NULL;
oyOptions_s *list_options = NULL,
            *backend_core = NULL,
            *data         = NULL;
oyConfig_s *device = NULL;

char *image_buffer = NULL;
char *icc_profile = NULL;
char *icc_profile_name = NULL;
int icc_profile_bytes;

int width, height, bps, size;

void print_sane_version()
{
   printf("SANE version %d.%d.%d\n\n",
          SANE_VERSION_MAJOR(version), SANE_VERSION_MINOR(version), SANE_VERSION_BUILD(version));
}

/// Initialise the device_list of sane devices
void init()
{
   int i = 0;
   oyConfigs_s *devices = NULL;
   /**1. Query Oyranos SANE backend for all avaliable devices**/
   /* Use the "command" -> "list" option
    * and also all other options that are supported by the backend */
   list_options = oyOptions_New(0);

   oyOptions_SetFromText(&list_options, CMM_BASE_REG OY_SLASH "command", "list", OY_CREATE_NEW);
   /*Sending a '0' for driver version will call sane_init and return us the sane version*/
   oyOptions_SetFromInt(&list_options, CMM_BASE_REG OY_SLASH "driver_version", version, 0, OY_CREATE_NEW);
   /*The value is not used in the following options*/
   oyOptions_SetFromText(&list_options, CMM_BASE_REG OY_SLASH "oyNAME_NAME", NULL, OY_CREATE_NEW);
   oyOptions_SetFromText(&list_options, CMM_BASE_REG OY_SLASH "device_context", NULL, OY_CREATE_NEW);
   oyOptions_SetFromText(&list_options, CMM_BASE_REG OY_SLASH "device_handle", NULL, OY_CREATE_NEW);
   /*Only a particular device has been asked*/
   if (device_name)
      oyOptions_SetFromText(&list_options, CMM_BASE_REG OY_SLASH "device_name", device_name, OY_CREATE_NEW);

   /*Now call Oyranos*/
   if (oyDevicesGet(OY_TYPE_STD, "scanner", list_options, &devices) != 0)
      exit(1);
   if (!devices)
      exit(1);

   /*Fill all the local variables with the returned options*/
   num_devices = oyConfigs_Count(devices);
   if (num_devices<1) {
      printf("No SANE devices found. Bye!");
      exit(0);
   } else
      printf("Found %d SANE device%s\n", num_devices, num_devices > 1 ? "s" : "");

   /*Get the SANE version from the first found device*/
   device = oyConfigs_Get(devices, 0);
   oyOptions_FindInt(device->data, CMM_BASE_REG "driver_version", 0, &version);
   oyConfig_Release(&device);
   print_sane_version();

   for (i = 0; i < num_devices; i++) {
      oyConfig_s *device = oyConfigs_Get(devices, i);
      oyOption_s *context_opt = oyConfig_Find(device, "device_context");
      int struct_size = 0;
      sane_device = (SANE_Device*)oyOption_GetData(context_opt, &struct_size, malloc);
      if (device_name) {
         oyOption_s *handle_opt = oyConfig_Find(device, "device_handle");
         sane_handle = (SANE_Handle*)oyOption_GetData(handle_opt, NULL, malloc);
         oyOption_Release(&handle_opt);
      }
      oyOption_Release(&context_opt);
      oyConfig_Release(&device);

      printf("[Device %d]\n", i);
      printf("name:\t%s\nvendor:\t%s\nmodel:\t%s\ntype:\t%s\n\n",
            device_list[i]->name, device_list[i]->vendor,
            device_list[i]->model, device_list[i]->type);

      free(sane_device);
      sane_device = NULL;
   }
}

void scan_it()
{
   SANE_Int bytes_left, bytes_written;
   SANE_Option_Descriptor *opt = NULL;
   SANE_Parameters params;

   char *buffer_aux = NULL;

   //1. Open the device
   status = sane_open(device_name, &device_handle);
   if (status != SANE_STATUS_GOOD) {
      printf("Cannot open device %s!\n", device_name);
      exit(1);
   }
   //2. Setup the device
   //This stage is not implemented in this simple programm
   status = sane_control_option(device_handle, 0, SANE_ACTION_GET_VALUE, &num_options, NULL);
   if (status != SANE_STATUS_GOOD) {
      printf("Cannot count device options!\n");
      exit(1);
   }
   printf("%s has %d options in total.\n", device_name, num_options);

   //An early implementation of color option handling, currently outside Oyranos
   OyInit_color_options(device_handle);
   OyPrint_color_options();
   //3. Acquire the scanned image
   status = sane_start(device_handle);
   if (status != SANE_STATUS_GOOD) {
      printf("Cannot start scanning!\n");
      exit(1);
   } else
      printf("sane_start()\n");
   //3.1 Take care of all scan parameters
   status = sane_get_parameters(device_handle, &params);
   if (status != SANE_STATUS_GOOD) {
      printf("Cannot get scanning parameters!\n");
      exit(1);
   }
   if (params.format != SANE_FRAME_RGB) {
      printf("Not interested in non RGB images. Bye\n!");
      exit(1);
   }
   if (params.lines == -1) {
      printf("Number of lines unknown. Bye!\n");
      exit(1);
   }
   height = params.lines;
   bps = params.depth;
   width = params.pixels_per_line;
   size = params.bytes_per_line * params.lines; //Total size of image in bytes
   if (params.bytes_per_line != 3 * (bps / 8) * width) {
      printf("Cannot cope with padded image formats!\n");
      exit(1);
   } else
      printf("ready to read image[%dx%d]@%dbits\n", width, height, bps);
   //3.2 Init image and fill with data.
   bytes_left = size;
   image_buffer = malloc(size);
   buffer_aux = image_buffer;
   if (image_buffer) {
      do {
         status = sane_read(device_handle, buffer_aux, bytes_left, &bytes_written);
         bytes_left -= bytes_written;
         buffer_aux += bytes_written;
         printf("size[%d]: %d bytes written, %d bytes left\n", size, bytes_written, bytes_left);
         if (status == SANE_STATUS_GOOD)
            continue;
         else if (status == SANE_STATUS_EOF && bytes_left == 0)
            break;
         else {
            printf("Something bad happened while reading image data. [status=%d]\n", status);
            if (bytes_left == 0) {
               printf("All seems OK, though. Let's ignore it.\n");
               break;
            } else
               exit(1);
         }
      } while (SANE_TRUE);
   } else {
      printf("Not enough memmory!\n");
      exit(1);
   }
   //3.3 Stop scanning
   sane_cancel(device_handle);

   //4. Close the device
   sane_close(device_handle);
}

#include <tiffio.h>
void save_tiff()
{
   char *tiffile = "out.tiff";
   int spp = 3;
   int h;

   TIFF *img = TIFFOpen(tiffile, "w");

   TIFFSetField(img, TIFFTAG_IMAGEDESCRIPTION, "GSoC 2009 test image");
   TIFFSetField(img, TIFFTAG_IMAGEWIDTH, width);
   TIFFSetField(img, TIFFTAG_IMAGELENGTH, height);
   TIFFSetField(img, TIFFTAG_SAMPLESPERPIXEL, spp);
   TIFFSetField(img, TIFFTAG_BITSPERSAMPLE, bps);
   TIFFSetField(img, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
   TIFFSetField(img, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
   TIFFSetField(img, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
   if (icc_profile) {
      printf("Attaching profile %s\n", icc_profile_name);
      TIFFSetField(img, TIFFTAG_ICCPROFILE, icc_profile_bytes, icc_profile);
   }

   tsize_t row_bytes = TIFFScanlineSize(img);
   for (h = 0; h < height; h++)
      TIFFWriteScanline(img, ((uint8 *) image_buffer) + h * row_bytes, h, 0);

   TIFFClose(img);
   printf("Saved image %s\n", tiffile);
}

void read_profile()
{
   FILE *profile = NULL;

   profile = fopen(icc_profile_name, "r");
   if (profile) {
      fseek(profile, 0L, SEEK_END);
      icc_profile_bytes = ftell(profile);
      rewind(profile);
      icc_profile = malloc(icc_profile_bytes);
      fread(icc_profile, 1, icc_profile_bytes, profile);
      fclose(profile);
   } else {
      printf("Cannot open file %s\n", icc_profile_name);
      exit(1);
   }
}

void cleanup()
{
   free(image_buffer);
   free(icc_profile);
   sane_exit();
}

void help()
{
   printf("\nUsage: %s [-d <device>] [-i [icc profile]]\n", argv[0]);
   printf("<device>:\n\t\tA sane device string\n<icc profile>:\n\t\t"
          "(a) A path to an *.ic[cm] file\n\t\t"
          "(b) If empty use Oyranos\n");
}

int main(int argc, char **argv)
{
   int arg;

   for (arg = 1; arg < argc; arg++) {
      if (strcmp(argv[arg], "-d") == 0)
         device_name = argv[++arg];
      else if (strcmp(argv[arg], "-i") == 0) {
         arg++;
         if (arg < argc && argv[arg][0] != '-')
            icc_profile_name = argv[arg];
         else
            icc_profile_name = "Oyranos";
      }
   }

   init();
   if (argc == 1 || !device_name) {
      help();
      return 0;
   }

   scan_it();
   if (icc_profile_name)
      read_profile();
   save_tiff();
   cleanup();

   return 0;
}
