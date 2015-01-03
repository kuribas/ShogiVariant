class ShogiBoard < Board
   def initialize
   end

   #find out if we need to promote
   #return next action
   def ask_promote(mv)
      if mv.can_promote and mv.can_stay
         case @promote
         when :no  : mv.promote = false
         when :yes : mv.promote = true
         when :ask
            cancel_move()
            case Platform.message_box(:question, "Promote piece?",
                                      "Don't promote", "Promote", :CANCEL)
            when 0 : mv.promote = false
            when 1 : mv.promote = true
            when 2 : return :cancel
            end
            return :redo
         end
      else mv.promote = mv.can_promote
      end
      return :finish
   end
end

class SmallShogiBoard < ShogiBoard
   def initialize
         
   end

   def do_board_move(movestr)
      move = @pos.parse_move(move_str)
      move
      
   end

   def setup_board
      for y in @type.width
         for x in @type.height
            piece, side = @pos[x, y]
            if piece.nil?
               put_piece(nil, x, y, MAINGRID)
            else
               piece = @type.piece_from_abbrev(piece)
               put_piece(theme[piece.sym, side], x, y, MAINGRID)
            end
         end
      end
      
      @pos.n_captures.times do |i|
      end
   end

   def add_captured(piece)
      
   end
   
   def remove_captured(piece)
         
   end

   def try_move(x, y, to_x, to_y)
      xr, yr = transform(x, y)
      to_xr, to_yr = transform(to_x, to_y)
      
      mv = @pos.legal_move(xr, yr, to_xr, to_yr)
      return nil if mv.nil?
      
      #if we are capturing save the piecetype of the captured piece
      old = get_piece(to_x, to_y, MAINGRID)
      piece = get_piece(x, y, MAINGRID)
      
      todo = ask_promote(mv)
      #cancel move if canceled by user
      return nil if todo == :cancel

      self.promote_to = piece.turned if mv.promote
      if todo == :redo
         move_piece(x, y, MAINGRID, to_x, to_y, MAINGRID, true)
      else #todo == :finish
         finish_move
      end
      
      add_captured(old.opposite) if old
      return mv
   end

   def try_drop(x, y, grid, to_x, to_y)
      piece = piece_at(x, y, grid)
      
      to_x, to_y = transform(to_x, to_y) if @flipped
      mv = @pos.legal_drop(piece.abbrev, to_x, to_y)
      
      return nil if mv.nil?
      
      remove_captured(piece)
      return mv
   end

   #callback when the user makes a move
   def user_move(x, y, grid, to_x, to_y, to_grid)
      #move pieces only to the main board
      return false if (to_grid != MAINGRID)

      if grid == MAINGRID
         mv = try_move(x, y, to_x, to_y)
      else
         mv = try_drop(x, y, grid, to_x, to_y)
      end
      
      return false if mv.nil?
      @pos.do_move(mv)
      
      board_moves(mv.to_s)
      return true
   end
end
