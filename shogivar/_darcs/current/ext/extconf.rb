require 'mkmf'
$LDFLAGS += "-lstdc++"
$CPPFLAGS += "-fno-rtti"

create_makefile("shogipos")
