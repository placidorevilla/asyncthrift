thrift_h.input = THRIFTS
thrift_h.output = gen-cpp/${QMAKE_FILE_IN_BASE}.h
thrift_h.commands = thrift --gen cpp:cob_style ${QMAKE_FILE_IN}
silent:thrift_h.commands = @echo compiling ${QMAKE_FILE_IN} && $$thrift_h.commands
thrift_h.variable_out = GENERATED_FILES
thrift_h.clean = gen-cpp/${QMAKE_FILE_IN_BASE}.h gen-cpp/${QMAKE_FILE_IN_BASE}_server.skeleton.cpp gen-cpp/${QMAKE_FILE_IN_BASE}_async_server.skeleton.cpp
thrift_h.config += target_predeps

thrift_types_h.input = THRIFTS
thrift_types_h.output = gen-cpp/${QMAKE_FILE_IN_BASE}_types.h
thrift_types_h.depends = gen-cpp/${QMAKE_FILE_IN_BASE}.h
thrift_types_h.commands = $$escape_expand(\\n)
thrift_types_h.variable_out = GENERATED_FILES

thrift_constants_h.input = THRIFTS
thrift_constants_h.output = gen-cpp/${QMAKE_FILE_IN_BASE}_constants.h
thrift_constants_h.depends = gen-cpp/${QMAKE_FILE_IN_BASE}.h
thrift_constants_h.commands = $$escape_expand(\\n)
thrift_constants_h.variable_out = GENERATED_FILES

thrift_cpp.input = THRIFTS
thrift_cpp.output = gen-cpp/${QMAKE_FILE_IN_BASE}.cpp
thrift_cpp.depends = gen-cpp/${QMAKE_FILE_IN_BASE}.h
thrift_cpp.commands = $$escape_expand(\\n)
thrift_cpp.variable_out = GENERATED_SOURCES

thrift_types_cpp.input = THRIFTS
thrift_types_cpp.output = gen-cpp/${QMAKE_FILE_IN_BASE}_types.cpp
thrift_types_cpp.depends = gen-cpp/${QMAKE_FILE_IN_BASE}.h
thrift_types_cpp.commands = $$escape_expand(\\n)
thrift_types_cpp.variable_out = GENERATED_SOURCES

thrift_constants_cpp.input = THRIFTS
thrift_constants_cpp.output = gen-cpp/${QMAKE_FILE_IN_BASE}_constants.cpp
thrift_constants_cpp.depends = gen-cpp/${QMAKE_FILE_IN_BASE}.h
thrift_constants_cpp.commands = $$escape_expand(\\n)
thrift_constants_cpp.variable_out = GENERATED_SOURCES

QMAKE_EXTRA_COMPILERS += thrift_h thrift_types_h thrift_constants_h thrift_cpp thrift_types_cpp thrift_constants_cpp

DEPENDPATH += gen-cpp
INCLUDEPATH += gen-cpp

