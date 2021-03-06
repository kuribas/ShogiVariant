# Superclass for all chesslike games.  Handles the communication
# between the player objects and the board, and tries to do as much as
# possible the handling for the underlying Platform::ChessBoard class.

class ChessBoard < Platform::ChessBoard
   def initialize(type, history = nil, setup = nil, observers = nil)
      @theme = Theme.new(self, type)
      super(*theme.board_args())
      
      @history = history || GameHistory.new
      @pos = Position.new(type)
      @pos.load_ggf(setup) if setup
      @observers = observer || Set.new

      @type = type
      @to_move = @pos.to_move
      @flipped = false
      self.player_side = NOSIDE
      self.active_side = @to_move
      setup_board
   end

   attr_accessor :history

   def set_position(setup)
      if setup
         @pos.load_ggf(setup)
      else
         @pos.init_setup
      end
      @to_move = @pos.to_move
      self.active_side = @to_move
      setup_board
   end

   def set_theme(theme)
      @theme = theme
      @theme.setup_board(self)
   end

   def start(player1, player2)
      @players = [player1, player2]
      set_player_side
   end

   def set_player_side
      if player_in_turn.on_board?
         self.player_side = @to_move
      elsif other_player.on_board?
         self.player_side = other_side
      else
         self.player_side = NOSIDE
      end
   end

   attr_accessor :promote
   def add_observer(player)
      @observers.add(player)
   end

   def remove_observer(player)
      @observers.delete(player)
   end

   def player1=(player)
      @players[0] = player
      player.set_board(self) if player.on_board?
      set_player_side
   end

   def player2=(player)
      @players[1] = player
      player.set_board(self) if player.on_board?
      set_player_side
   end

   #called by the player object when it wants to make a move.
   #It's the responsibility from the player to check for
   #validity of the move.
   
   def player_moves(player, movestr)
      raise "player moved in stopped game" if finished?
      raise "player moved when not its turn" if player_in_turn != player
      
      cont = other_player.notify_move(movestr) unless @players[0] == @players[1]
      @observers.each { |obs| obs.notify_move(movestr) }

      do_player_move(movestr) #defined by subcclass
      switch_side(cont)
   end

   def player_requests(player, request)
      raise "player made request in finished game" if finished?
      raise "request not made by player" unless @players.include?(player)
      
      answer = other_player.do_request(request)
      return nil unless answer
      
      @observers.each { |obs| obs.notify_request(request) }
      return answer
   end

   def other_side
      @to_move && (@to_move + 1) % 2
   end
      
   def player_in_turn
      @players[@to_move]
   end

   def other_player
      @players[other_side]
   end

   def finished?
      @to_move.nil?
   end

   #invoked when the user makes a move using the board.  Inform the
   #player object of the move.
   # The validity of the move should be checked by the subclass.
   
   def board_moves(move)
      movestr = move.to_s
      cont = other_player.notify_move(movestr) unless @players[0] == @players[1]
      @observers.each { |obs| obs.notify_move(movestr) }

      player_in_turn.do_move(movestr)
      @pos.do_move(move)
      
      switch_side(cont)
   end

   def switch_side(cont)
      if cont
         raise "Switching side in finished game" if finished?
         @to_move = other_side
         self.player_side = @to_move if @players[@to_move].on_board?
         self.active_side = @to_move
      else
         self.player_side = NOSIDE
         @to_move = nil
      end
   end

   def init_board
      @theme.init_board
   end
end
