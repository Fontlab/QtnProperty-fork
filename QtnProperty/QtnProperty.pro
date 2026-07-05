include(../Internal/TargetConfig.pri)

TARGET = QtnProperty
TEMPLATE = lib
VERSION = 2.0.3

CONFIG(debug,debug|release){
# QMAKE_CXXFLAGS_DEBUG += -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -O1
# QMAKE_CFLAGS_DEBUG   += -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -O1
# QMAKE_LFLAGS_DEBUG   += -fsanitize=address
}
else
{
}

qtnproperty_dynamic {
    DEFINES += QTN_DYNAMIC_LIBRARY
    macx {
        QMAKE_SONAME_PREFIX = @rpath
    }

    CONFIG += shared
} else {
    CONFIG += static
}

include(./QtnProperty.pri)
