/*
  rbgtkcboard.c - ruby wrapper code for gtk_cboard
  http://gcboard.sourceforge.net
  Copyright (C) 2004 Kristof Bastiaensen
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <ruby.h>
#include <string>
#include "ShogiImpl.H"
#include <sstream>

#define IS_SMALL_BOARD(board) ((board)->get_type()->num < 11)
#define IS_LARGE_BOARD(board) ((board)->get_type()->num >= 11)

#define LARGE_BOARD(board) ((LargeBoard *)board)
#define SMALL_BOARD(board) ((SmallBoard *)board)

typedef VALUE(*ruby_method)(ANYARGS);

static ID id_x, id_y, id_to_s;
static VALUE MoveClass, PositionClass, TypeClass, PieceClass, Types;

extern const SmallBoard::Type *small_types[];
extern const Board::Type *large_types[];

struct ShogiPiece
{
   const Piece *piece;
   VALUE promote_to;
   VALUE promote_from;
};

struct ShogiType
{
   const Board::Type *type;
   VALUE capturable;
   VALUE pieces;
};

static ShogiType shogitypes[21];

static int get_int_prop(VALUE obj, ID prop)
{
   return NUM2INT(rb_funcall(obj, prop, 0));
}

static void free_board(Board *board)
{
   delete board;
}

static void free_move(Board::Move *move)
{
   delete move;
}

static void free_piece(ShogiPiece *piece)
{
   delete piece;
}

static void mark_type(ShogiType *type)
{
   if(type->capturable != Qnil)
      rb_gc_mark(type->capturable);
   if(type->pieces != Qnil)
      rb_gc_mark(type->pieces);
}

static void mark_piece(ShogiPiece *piece)
{
   if(piece->promote_to != Qnil)
      rb_gc_mark(piece->promote_to);
   if(piece->promote_from != Qnil)
      rb_gc_mark(piece->promote_from);
}

static VALUE
move_new(VALUE klass, Board *board)
{
   VALUE obj;
   Board::Move *mv = new Board::Move(board->get_type());

   if(!mv) return Qnil;
   obj = Data_Wrap_Struct(klass, NULL,
			  free_move, mv);
   return obj;
}

static VALUE
move_set_promote(VALUE v_move, VALUE promote)
{
   Board::Move *move;
   
   if(promote == Qnil || promote == Qfalse)
   {
       if(rb_iv_get(v_move, "@can_stay") == Qfalse)
	  rb_raise(rb_eTypeError,
		   "Cannot set promote to false: move has to promote on the next move.");
   }
   else
   {
       if(rb_iv_get(v_move, "@can_promote") == Qfalse)
	  rb_raise(rb_eTypeError,
		   "Cannot set promote to true: move cannot promote on the next move.");
   }
      
   Data_Get_Struct(v_move, Board::Move, move);
   move->promote = (promote == Qnil || promote == Qfalse ? 0 : 1);

   return promote;
}

static VALUE
move_get_promote(VALUE v_move)
{
   Board::Move *move;
   
   Data_Get_Struct(v_move, Board::Move, move);
   return (move->promote ? Qtrue : Qfalse);
}

static VALUE
move_get_type(VALUE v_move)
{
   Board::Move *move;

   Data_Get_Struct(v_move, Board::Move, move);
   return INT2FIX(move->type);
}

static VALUE
move_get_to(VALUE v_move)
{
   Board::Move *move;

   Data_Get_Struct(v_move, Board::Move, move);
   return rb_ary_new3(2, INT2FIX(move->to.x), INT2FIX(move->to.y));
}

static VALUE
move_get_to2(VALUE v_move)
{
   Board::Move *move;
   
   Data_Get_Struct(v_move, Board::Move, move);
   if(move->type != Board::Move::DOUBLE &&
      move->type != Board::Move::TRIPLE)
      rb_raise(rb_eTypeError, "Not a composite move.");
   else
      return rb_ary_new3(2, INT2FIX(move->to2.x), INT2FIX(move->to2.y));
}

static VALUE
move_get_to3(VALUE v_move)
{
   Board::Move *move;

   Data_Get_Struct(v_move, Board::Move, move);
   if(move->type != Board::Move::TRIPLE)
      rb_raise(rb_eTypeError, "Not a triple move.");
   else
      return rb_ary_new3(2, INT2FIX(move->to3.x), INT2FIX(move->to3.y));
}

static VALUE
move_get_piece(VALUE v_move)
{
   Board::Move *move;

   Data_Get_Struct(v_move, Board::Move, move);
   if(move->type != Board::Move::DROP)
      rb_raise(rb_eTypeError, "Move is not a dropping move.");
   else
      return rb_str_new2(move->bdtype->pieces[move->piece]->name);
}

static VALUE
move_get_from(VALUE v_move)
{
   Board::Move *move;

   Data_Get_Struct(v_move, Board::Move, move);
   if(move->type == Board::Move::DROP)
      rb_raise(rb_eTypeError, "Dropping move has no source square.");
   else
      return rb_ary_new3(2, INT2FIX(move->from.x), INT2FIX(move->from.y));
}
  

static VALUE pos_new(VALUE v_class, VALUE type)
{
   Board *board;
   VALUE obj;
   int typenum;
   
   if(FIXNUM_P(type))
   {
       if(NUM2INT(type) < 0 ||
	  NUM2INT(type) > 20)
       {
	   rb_raise(rb_eArgError, "wrong type: %i", NUM2INT(type));
	   return Qnil;
       }
       typenum = NUM2INT(type);
   } else if(rb_obj_is_kind_of(type, TypeClass))
   {
       ShogiType *ctype;
       Data_Get_Struct(type, ShogiType, ctype);
       typenum = ctype->type->num;
   } else {
       rb_raise(rb_eTypeError, "not a ShogiPosition::Type");
       return Qnil;
   }

   board = Board::new_board(typenum);
   if(!board) return Qnil;
       
   board->init_setup();
   obj = Data_Wrap_Struct(v_class, NULL,
			  free_board, board);
   
   return obj;
}

static VALUE pos_init_setup(VALUE self)
{
   Board *board;

   Data_Get_Struct(self, Board, board);
   board->init_setup();

   return Qnil;
}

static VALUE pos_get_type(VALUE self)
{
   Board *board;

   Data_Get_Struct(self, Board, board);
   return rb_ary_entry(Types, board->get_type()->num);
}

static VALUE pos_finished(VALUE self)
{
   Board *board;
   
   Data_Get_Struct(self, Board, board);
   if (board->is_finished())
      return INT2FIX(board->get_result());
   else
      return Qnil;
}

static VALUE pos_legal_move(VALUE self, VALUE from, VALUE to)
{
   VALUE v_move;
   Board::Move *move;
   Board *board;
   Point pt;
   int prom_info;

   Data_Get_Struct(self, Board, board);

   v_move = move_new(MoveClass, board);
   Data_Get_Struct(v_move, Board::Move, move);
   
   move->type = Board::Move::SIMPLE;
   pt.x = get_int_prop(from, id_x);
   pt.y = get_int_prop(from, id_y);
   move->from = pt;

   pt.x = get_int_prop(to, id_x);
   pt.y = get_int_prop(to, id_y);
   move->to = pt;

   if(!board->legal_move(*move))
      return Qnil;

   prom_info = board->can_promote();
   rb_iv_set(v_move, "@can_promote", prom_info & 1 ? Qtrue : Qfalse);
   rb_iv_set(v_move, "@can_stay", prom_info & 2 ? Qtrue : Qfalse);
   move->promote = 0;

   if(IS_LARGE_BOARD(board) &&
      LARGE_BOARD(board)->is_partial())
   {
       rb_iv_set(v_move, "@partial", Qtrue);
   } else
      rb_iv_set(v_move, "@partial", Qfalse);

   return v_move;
}

static const Piece* get_piece(Board *board, VALUE v_piece)
{
   if(TYPE(v_piece) != T_STRING)
   {
       v_piece = rb_funcall(v_piece, id_to_s, 0);
       Check_Type(v_piece, T_STRING);
   }
       
   return board->lookup_piece(STR2CSTR(v_piece));
}

static VALUE
pos_legal_drop(VALUE self, VALUE v_piece, VALUE v_to)
{
   VALUE v_move;
   Board::Move *move;
   Board *board;
   Point pt;
   const char *piecename;
   const Piece *piece;
   
   Data_Get_Struct(self, Board, board);
   
   v_move = move_new(MoveClass, board);
   Data_Get_Struct(v_move, Board::Move, move);

   move->type = Board::Move::DROP;
   piece = get_piece(board, v_piece);
   if(piece == NULL)
      return Qnil;
   move->piece = piece->num;

   pt.x = get_int_prop(v_to, id_x);
   pt.y = get_int_prop(v_to, id_y);
   move->to = pt;

   if(!board->legal_move(*move))
      return Qnil;

   rb_iv_set(v_move, "@can_promote", Qfalse);
   rb_iv_set(v_move, "@can_stay", Qtrue);
   rb_iv_set(v_move, "@partial", Qfalse);
   
   return v_move;
}

static VALUE pos_legal_cont(VALUE self, VALUE v_move, VALUE v_to)
{
   Board::Move *move;
   Board *board;
   Point pt;
   int prom_info;

   Data_Get_Struct(self, Board, board);

   if(!rb_obj_is_kind_of(v_move, MoveClass))
      rb_raise(rb_eTypeError, "not a Board::Move");
   
   Data_Get_Struct(v_move, Board::Move, move);

   switch(move->type)
   {
     case Board::Move::SIMPLE:
	move->type = Board::Move::DOUBLE;
	pt.x = get_int_prop(v_to, id_x);
	pt.y = get_int_prop(v_to, id_y);
	move->to2 = pt;
	break;
     case Board::Move::DOUBLE:
	move->type = Board::Move::TRIPLE;
	pt.x = get_int_prop(v_to, id_x);
	pt.y = get_int_prop(v_to, id_y);
	move->to3 = pt;
	break;
     default:
	return Qnil;
   }

   if(!board->legal_move(*move))
   {
       //reset
       move->type = (move->type == Board::Move::TRIPLE ?
		     Board::Move::DOUBLE : Board::Move::SIMPLE);
       return Qnil;
   }

   prom_info = board->can_promote();
   rb_iv_set(v_move, "@can_promote", prom_info & 1 ? Qtrue : Qfalse);
   rb_iv_set(v_move, "@can_stay", prom_info & 2 ? Qtrue : Qfalse);

   if(IS_LARGE_BOARD(board) &&
      (LARGE_BOARD(board)->is_partial()))
   {
       rb_iv_set(v_move, "@partial", Qtrue);
   } else
      rb_iv_set(v_move, "@partial", Qfalse);

   return v_move;
}

static void
convert_igui_move(Board::Move *move)
{
   move->type = Board::Move::DOUBLE;
   move->to2 = move->from;
}

static void
convert_pass_move(Board *board, Board::Move *mv)
{
   int x, y;

   mv->type = Board::Move::DOUBLE;
   for(y = -1; y <= 1; y++)
      for(x = -1; x <= 1; x++)
	 if((x || y) &&
	    board->on_board(mv->from.x + x, mv->from.y + y) &&
	    board->empty_square(mv->from.x + x, mv->from.y + y))
	 {
	     mv->to.x = mv->from.x + x;
	     mv->to.y = mv->from.y + y;
	     mv->to2 = mv->from;
	     return;
	 }
}

static VALUE
pos_parse_move(VALUE self, VALUE v_str)
{
   VALUE v_move;
   Board::Move *move;
   Board *board;
   string s;
   
   Data_Get_Struct(self, Board, board);
   
   v_move = move_new(MoveClass, board);
   Data_Get_Struct(v_move, Board::Move, move);
   Check_Type(v_str, T_STRING);

   s = STR2CSTR(v_str);

   if(!move->parse(s) || !board->one_legal_move(*move))
      return Qnil;

   /* Convert an IGUI or a PASS move into a DOUBLE move,
      which is easier for handle for the interface.
   */
   if(move->type == Board::Move::IGUI)
      convert_igui_move(move);
   else if(move->type == Board::Move::PASS)
      convert_pass_move(board, move);
   
   return v_move;
}

static VALUE pos_dup(VALUE self)
{
   Board *board, *new_brd;
   VALUE obj;

   Data_Get_Struct(self, Board, board);
   new_brd = board->duplicate();

   obj = Data_Wrap_Struct(PositionClass, NULL,
			  free_board, new_brd);
   return obj;
}

static VALUE pos_do_move(VALUE self, VALUE v_move)
{
   Board *board;
   Board::Move *move;

   Data_Get_Struct(self, Board, board);

   if(!rb_obj_is_kind_of(v_move, MoveClass))
      rb_raise(rb_eTypeError, "not a Board::Move");
   Data_Get_Struct(v_move, Board::Move, move);
   board->do_move(*move);

   return Qnil;
}

static VALUE pos_burns(VALUE self)
{
   Board *board;
   VALUE *values;
   int n_burns, i;
   Point pt;

   Data_Get_Struct(self, Board, board);
   
   if(IS_SMALL_BOARD(board))
      return Qnil;

   n_burns = LARGE_BOARD(board)->n_burns;
   if(n_burns == 0)
      return Qnil;

   values = ALLOCA_N(VALUE, n_burns);
   for(i = 0; i < n_burns; i++)
   {
       pt = LARGE_BOARD(board)->burned_pieces[i];
       values[i] = rb_ary_new3(2, INT2FIX(pt.x), INT2FIX(pt.y));
   }
   
   return rb_ary_new4(n_burns, values);
}

static VALUE pos_n_captures(VALUE self, VALUE v_piece, VALUE v_side)
{
   Board *board;
   const Piece *piece;
   int side;
   
   Data_Get_Struct(self, Board, board);

   if(IS_LARGE_BOARD(board))
      return Qnil;

   piece = get_piece(board, v_piece);
   side = NUM2INT(v_side);

   if(!piece)
      return Qnil;
   if(side < 0 || side > 1)
      return Qnil;

   return INT2FIX(SMALL_BOARD(board)->get_capture(piece->num, side));
}

static VALUE type_aref(VALUE, VALUE);
static VALUE type_pieces(VALUE);

static VALUE pos_at(VALUE self, VALUE square)
{
   Board *board;
   Point pt;
   int side;
   VALUE piece;

   Data_Get_Struct(self, Board, board);

   pt.x = get_int_prop(square, id_x);
   pt.y = get_int_prop(square, id_y);

   if(!board->on_board(pt)) return Qnil;

   side = board->side_at(pt);
   if(side == Board::NO_SIDE) return Qnil;

   piece = rb_ary_entry(type_pieces(pos_get_type(self)),
			board->piece_at(pt)->num);

   return rb_ary_new3(2, piece, INT2FIX(side));
}

static void
read_pos_array(Board *board, VALUE arr, Board::Square *pos)
{
   int i, j;
   
   Check_Type(arr, T_ARRAY);

   if(RARRAY(arr)->len != board->get_type()->height)
      rb_raise(rb_eArgError, "Invalid height for Array");

   for(i = 0; i < RARRAY(arr)->len; i++)
   {
       VALUE row = RARRAY(arr)->ptr[i];

       Check_Type(row, T_ARRAY);
       if(RARRAY(row)->len != board->get_type()->width)
	  rb_raise(rb_eArgError, "Invalid width for row");

       for(j = 0; j < RARRAY(row)->len; j++)
       {
	   const Piece *piece;
	   int side, index;

	   VALUE square = RARRAY(row)->ptr[j];
	   if(square == Qnil)
	   {
	       index = i * board->get_type()->height + j;
	       pos[index].piece = 0;
	       pos[index].side = Board::NO_SIDE;
	   }
	   else
	   {
	       Check_Type(square, T_ARRAY);
	       if(RARRAY(square)->len != 2)
		  rb_raise(rb_eArgError, "Square needs two elements");

	       Check_Type(RARRAY(square)->ptr[1], T_FIXNUM);
	       piece = get_piece(board, RARRAY(square)->ptr[0]);
	       
	       if(!piece) rb_raise(rb_eArgError, "Invalid piece");

	       side = FIX2INT(RARRAY(square)->ptr[1]);
	       if(side != 0 && side != 1)
		  rb_raise(rb_eArgError, "Invalid side");

	       index = i * board->get_type()->height + j;
	       pos[index].piece = piece->num;
	       pos[index].side = side;
	   }
       }
   }
}

static void
read_pos_capts(int *capts, VALUE capt, int n_capts)
{
   int i;
   
   Check_Type(capt, T_ARRAY);
   if (RARRAY(capt)->len != n_capts)
      rb_raise(rb_eArgError, "Incorrect number of captures");

   for(i = 0; i < n_capts; i++)
   {
       Check_Type(RARRAY(capt)->ptr[i], T_FIXNUM);
       capts[i] = FIX2INT(RARRAY(capt)->ptr[i]);
   }
}

static inline VALUE
set_pos_small(int argc, VALUE *argv, SmallBoard *board)
{
   VALUE brd, capt1, capt2;
   Board::Square *pos;
   int *captures[2];
   VALUE to_move;
   int n_capt;

   rb_scan_args(argc, argv, "4", &brd, &capt1, &capt2, &to_move);
   pos = ALLOCA_N(Board::Square,
		  board->get_type()->width *
		  board->get_type()->height);

   n_capt = ((SmallBoard::Type*)board->get_type())->captures_size;

   captures[0] = ALLOCA_N(int, n_capt);
   captures[1] = ALLOCA_N(int, n_capt);
   
   read_pos_array(board, brd, pos);
   read_pos_capts(captures[0], capt1, n_capt);
   read_pos_capts(captures[1], capt2, n_capt);

   board->set_pos(pos, captures[0], captures[1], NUM2INT(to_move));
   
   return Qnil;
}

static inline VALUE
set_pos_large(int argc, VALUE *argv, LargeBoard *board)
{
   VALUE brd, to_move;
   Board::Square *pos;
   
   rb_scan_args(argc, argv, "2", &brd, &to_move);
   pos = ALLOCA_N(Board::Square,
		  board->get_type()->width *
		  board->get_type()->height);
   
   read_pos_array(board, brd, pos);
   board->set_pos(pos, NUM2INT(to_move));
   
   return Qnil;
}

static VALUE pos_set_pos(int argc, VALUE *argv, VALUE self)
{
   Board *board;
   
   Data_Get_Struct(self, Board, board);

   if(IS_SMALL_BOARD(board))
      return set_pos_small(argc, argv, SMALL_BOARD(board));
   else
      return set_pos_large(argc, argv, LARGE_BOARD(board));
}

static VALUE pos_write(VALUE self)
{
   Board *board;
   ostringstream os;
   
   Data_Get_Struct(self, Board, board);
   board->write(os);

   return rb_str_new2(os.str().c_str());
}

static VALUE pos_write_ggf(VALUE self, VALUE one_line)
{
   Board *board;
   ostringstream os;
   
   Data_Get_Struct(self, Board, board);
   board->write_ggf(os, one_line != Qfalse && one_line != Qnil);

   return rb_str_new2(os.str().c_str());
}

static VALUE pos_read_ggf(VALUE self, VALUE v_str)
{
   Board *board;
   string str;
   istringstream is;

   Data_Get_Struct(self, Board, board);

   Check_Type(v_str, T_STRING);
   str = STR2CSTR(v_str);
   is.str(str);
   
   return (board->read_pos(is) ? Qtrue : Qfalse);
}

static VALUE register_pieces(const Board::Type *type)
{
   int i;
   VALUE *pieces;
   ShogiPiece *piece;

   pieces = ALLOCA_N(VALUE, type->nPieces);
   
   for(i = 0; i < type->nPieces; i++)
   {
       piece = new ShogiPiece;
       piece->piece = type->pieces[i];
       piece->promote_to = Qnil;
       piece->promote_from = Qnil;
       pieces[i] = Data_Wrap_Struct(PieceClass, mark_piece, free_piece, piece);
   }

   //resolve recursive references
   for(i = 0; i < type->nPieces; i++)
   {
       Data_Get_Struct(pieces[i], ShogiPiece, piece);
       if(type->pieces[i]->promote_to)
	  piece->promote_to = pieces[type->pieces[i]->promote_to->num];

       if(type->pieces[i]->promote_from)
	  piece->promote_from = pieces[type->pieces[i]->promote_from->num];
   }

   return rb_ary_new4(type->nPieces, pieces);
}

static VALUE register_types()
{
   VALUE types[21];
   int i;
   VALUE pieces;
   VALUE range[2];
   VALUE empty = rb_ary_new2(0);
   const Board::Type *type;

   range[0] = INT2FIX(0);

   for(i = 0; i < 11; i++) {
       range[1] = INT2FIX(small_types[i]->captures_size);
       type = &(small_types[i]->type);
       pieces = register_pieces(type);
       
       shogitypes[i].type = type;
       shogitypes[i].pieces = pieces;
       if (FIX2INT(range[1]) > 0)
	  shogitypes[i].capturable = rb_ary_aref(2, range, pieces);
       else
          // the following actually does the same, but we share the
	  // empty array like this.
	  shogitypes[i].capturable = empty;
       types[i] = Data_Wrap_Struct(TypeClass, mark_type, NULL, &shogitypes[i]);
   }

   for(i = 11; i < 21; i++) {
       shogitypes[i].type = large_types[i - 11];
       shogitypes[i].pieces = Qnil;
       shogitypes[i].capturable = empty;
       types[i] = Data_Wrap_Struct(TypeClass, mark_type, NULL, &shogitypes[i]);
       // Why putting this here?  Because this way the array with pieces can
       // be marked for the Garbage Collector.  If it is saved only in a global
       // variable the GC maybe cannot find it.
       shogitypes[i].pieces = register_pieces(large_types[i - 11]);
   }

   return rb_ary_new4(21, types);
}

static VALUE type_name(VALUE self)
{
   ShogiType *type;
   Data_Get_Struct(self, ShogiType, type);
   return rb_str_new2(type->type->name);
}

static VALUE type_width(VALUE self)
{
   ShogiType *type;
   Data_Get_Struct(self, ShogiType, type);
   return INT2FIX(type->type->width);
}

static VALUE type_height(VALUE self)
{
   ShogiType *type;
   Data_Get_Struct(self, ShogiType, type);
   return INT2FIX(type->type->height);
}

static VALUE type_pieces(VALUE self)
{
   ShogiType *type;
   Data_Get_Struct(self, ShogiType, type);
   return type->pieces;
}

static VALUE type_to_i(VALUE self)
{
   ShogiType *type;
   Data_Get_Struct(self, ShogiType, type);
   return INT2FIX(type->type->num);
}

static VALUE type_capturable(VALUE self)
{
   ShogiType *type;
   Data_Get_Struct(self, ShogiType, type);
   return type->capturable;
}

static VALUE type_aref(VALUE self, VALUE piecename)
{
   int num;
   ShogiType *type;

   Data_Get_Struct(self, ShogiType, type);
   num = type->type->num_from_name(STR2CSTR(piecename));

   if (num == -1)
      return Qnil;
   else
      return rb_ary_entry(type->pieces, num);
}

static VALUE piece_name(VALUE self)
{
   ShogiPiece *piece;
   Data_Get_Struct(self, ShogiPiece, piece);
   return(rb_str_new2(piece->piece->long_name));
}

static VALUE piece_abbrev(VALUE self)
{
   ShogiPiece *piece;
   Data_Get_Struct(self, ShogiPiece, piece);
   return(rb_str_new2(piece->piece->name));
}
static VALUE piece_promote_to(VALUE self)
{
   ShogiPiece *piece;
   Data_Get_Struct(self, ShogiPiece, piece);
   return piece->promote_to;
}

static VALUE piece_promote_from(VALUE self)
{
   ShogiPiece *piece;
   Data_Get_Struct(self, ShogiPiece, piece);
   return piece->promote_from;
}

static VALUE piece_to_i(VALUE self)
{
   ShogiPiece *piece;
   Data_Get_Struct(self, ShogiPiece, piece);
   return INT2FIX(piece->piece->num);
}

static VALUE piece_rank(VALUE self)
{
   ShogiPiece *piece;
   Data_Get_Struct(self, ShogiPiece, piece);
   return INT2FIX(piece->piece->rank);
}

extern "C" void Init_shogipos()
{
   id_x = rb_intern("x");
   id_y = rb_intern("y");
   id_to_s = rb_intern("to_s");
   
   PositionClass = rb_define_class("ShogiPosition", rb_cObject);
   MoveClass = rb_define_class_under(PositionClass, "Move", rb_cObject);
   TypeClass = rb_define_class_under(PositionClass, "Type", rb_cObject);
   PieceClass = rb_define_class_under(PositionClass, "Piece", rb_cObject);

   rb_define_method(MoveClass, "promote", (ruby_method)move_get_promote, 0);
   rb_define_method(MoveClass, "promote=", (ruby_method)move_set_promote, 1);
   rb_define_method(MoveClass, "type", (ruby_method)move_get_type, 0);
   rb_define_method(MoveClass, "to", (ruby_method)move_get_to, 0);
   rb_define_method(MoveClass, "to2", (ruby_method)move_get_to2, 0);
   rb_define_method(MoveClass, "to3", (ruby_method)move_get_to3, 0);
   rb_define_method(MoveClass, "from", (ruby_method)move_get_from, 0);
   rb_define_method(MoveClass, "piece", (ruby_method)move_get_piece, 0);
   rb_define_const(MoveClass, "SIMPLE", INT2FIX(1));
   rb_define_const(MoveClass, "DROP", INT2FIX(2));
   rb_define_const(MoveClass, "DOUBLE", INT2FIX(5));
   rb_define_const(MoveClass, "TRIPLE", INT2FIX(6));
   rb_funcall(MoveClass, rb_intern("attr_reader"), 3,
	      ID2SYM(rb_intern("can_promote")),
	      ID2SYM(rb_intern("can_stay")),
	      ID2SYM(rb_intern("partial")));

   rb_define_singleton_method(PositionClass, "new", (ruby_method)pos_new, 1);   
   rb_define_method(PositionClass, "init_setup", (ruby_method)pos_init_setup, 0);
   rb_define_method(PositionClass, "finished", (ruby_method)pos_finished, 0);
   rb_define_method(PositionClass, "type", (ruby_method)pos_get_type, 0);
   rb_define_method(PositionClass, "legal_move", (ruby_method)pos_legal_move, 2);
   rb_define_method(PositionClass, "legal_drop", (ruby_method)pos_legal_drop, 2);
   rb_define_method(PositionClass, "legal_cont", (ruby_method)pos_legal_cont, 2);
   rb_define_method(PositionClass, "parse_move", (ruby_method)pos_parse_move, 1);
   rb_define_method(PositionClass, "dup", (ruby_method)pos_dup, 0);
   rb_define_method(PositionClass, "do_move", (ruby_method)pos_do_move, 1);
   rb_define_method(PositionClass, "burns", (ruby_method)pos_burns, 0);
   rb_define_method(PositionClass, "n_captures", (ruby_method)pos_n_captures, 2);
   rb_define_method(PositionClass, "[]", (ruby_method)pos_at, 1);
   rb_define_method(PositionClass, "set_pos", (ruby_method)pos_set_pos, -1);
   rb_define_method(PositionClass, "write", (ruby_method)pos_write, 0);
   rb_define_method(PositionClass, "write_ggf", (ruby_method)pos_write_ggf, 1);
   rb_define_method(PositionClass, "read_ggf", (ruby_method)pos_read_ggf, 1);

   rb_define_method(TypeClass, "name", (ruby_method)type_name, 0);
   rb_define_method(TypeClass, "to_s", (ruby_method)type_name, 0);
   rb_define_method(TypeClass, "width", (ruby_method)type_width, 0);
   rb_define_method(TypeClass, "height", (ruby_method)type_height, 0);
   rb_define_method(TypeClass, "pieces", (ruby_method)type_pieces, 0);
   rb_define_method(TypeClass, "to_i", (ruby_method)type_to_i, 0);
   rb_define_method(TypeClass, "capturable", (ruby_method)type_capturable, 0);
   rb_define_method(TypeClass, "[]", (ruby_method)type_aref, 1);

   rb_define_method(PieceClass, "name", (ruby_method)piece_name, 0);
   rb_define_method(PieceClass, "to_s", (ruby_method)piece_abbrev, 0);
   rb_define_method(PieceClass, "abbrev", (ruby_method)piece_abbrev, 0);
   rb_define_method(PieceClass, "promote_to", (ruby_method)piece_promote_to, 0);
   rb_define_method(PieceClass, "promote_from", (ruby_method)piece_promote_from, 0);
   rb_define_method(PieceClass, "to_i", (ruby_method)piece_to_i, 0);
   rb_define_method(PieceClass, "rank", (ruby_method)piece_rank, 0);

   Types = register_types();
   rb_define_const(PositionClass, "TYPES", Types);
}
