require 'ext/shogipos'

class Position
   class Move
      attr_reader :can_promote, :can_stay
      attr_reader :partial
      attr_accessor :descr
      attr_accessor :to_s
      attr_accessor :undo_info
   end

   def update_move(move)

   end

end
