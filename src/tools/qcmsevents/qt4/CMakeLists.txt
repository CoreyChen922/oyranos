# X Color Management event observer
FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui)
IF(QT_FOUND)
  SET( HAVE_QT 1 )
  INCLUDE_DIRECTORIES( ${QT_INCLUDE_DIRS} )
  LINK_DIRECTORIES( ${QT_LIBRARY_DIRS} )
ENDIF(QT_FOUND)

IF( HAVE_QT AND UNIX AND NOT APPLE AND NOT ${CMAKE_SYSTEM_NAME} MATCHES Android )
  IF(QT4_FOUND)
    INCLUDE(   ${QT_USE_FILE}   )

    SET( qcmsevents_SRCS
         ../qcmsevents.cpp )
    SET( qcmsevents_MOC_HEADERS
         ../qcmsevents.h )
    SET( qcmsevents_RCCS
         ../qcmsevents.qrc )
    QT4_ADD_RESOURCES( qcmsevents_RCC_SRCS ${qcmsevents_RCCS} )
    QT4_WRAP_CPP( qcmsevents_SRCS ${qcmsevents_MOC_HEADERS} )

    ADD_EXECUTABLE( qcmsevents-qt4 ${qcmsevents_SRCS} ${qcmsevents_RCC_SRCS} )
    TARGET_LINK_LIBRARIES ( qcmsevents-qt4 ${TARGET_LIB} ${EXTRA_LIBS}
                    ${QT_LIBRARIES} ${XCM_LIBRARIES} )
    INSTALL( TARGETS qcmsevents-qt4
             RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
    INSTALL( FILES ../qcmsevents-qt4-applet.desktop DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications )
    INSTALL( FILES ../qcmsevents-qt4.svg DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/pixmaps )
  ENDIF()
ENDIF()

