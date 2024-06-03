set confirm off
set print thread-events off
set output-radix 0x08

# Display in PDP10 000000,,000000 style
define oo
  p/o {($arg0 >> 18) & 0777777, $arg0 & 0777777}
end
