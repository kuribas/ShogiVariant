class ShogiPosition
   class Type
      to_s
      name
      [] => piece
      width
      height
      pieces
      to_i
      capturable
   end

   class ShogiPiece
      to_s
      name
      abbrev
      promote_to
      promote_from
      to_i
   end
end
