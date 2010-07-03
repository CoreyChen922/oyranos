{% include "source_file_header.txt" %}

#ifndef OY_{{ class.baseName|upper }}_S__H
#define OY_{{ class.baseName|upper }}_S__H

#include <oyranos_object.h>

typedef struct {{ class.privName }} {{ class.privName }};

/** @internal
 *  @struct   {{ class.privName }}
 *  @brief    {{ class.brief }}
 *  @ingroup  {{ class.group }}
 *  @extends  {{ class.parentName }}
 *
 *  @version Oyranos: x.x.x
 *  @since   YYYY/MM/DD (Oyranos: x.x.x)
 *  @date    YYYY/MM/DD
 */
struct {{ class.privName }} {
{% block BaseMembers %}
/* Include "Struct.members.h" { */
{% include "Struct.members.h" %}
/* } Include "Struct.members.h" */
{% endblock %}
{% block ChildMembers %}{% endblock %}
};

{% block GeneralPrivateMethodsDeclarations %}
{{ class.privName }}*
  oy{{ class.baseName }}_New_( oyObject_s_ object );
{{ class.privName }}*
  oy{{ class.baseName }}_Copy_( {{ class.privName }} *obj, oyObject_s_ object);
{{ class.privName }}*
  oy{{ class.baseName }}_Copy__( {{ class.privName }} *obj, oyObject_s_ object);
int
  oy{{ class.baseName }}_Release_( {{ class.privName }} **obj );
{% endblock %}

{% block SpecificPrivateMethodsDeclarations %}
/* Include "{{ class.private_methods_declarations_h }}" { */
{% include class.private_methods_declarations_h %}
/* } Include "{{ class.private_methods_declarations_h }}" */
{% endblock %}

#endif /* OY_{{ class.baseName|upper }}_S__H */
