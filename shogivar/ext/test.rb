require 'shogipos'

Point = Struct.new(:x, :y)

class Position::Move
   attr_reader :can_promote, :can_stay, :partial
end

$p = Position.new(0)
puts $p.write

