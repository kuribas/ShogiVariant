((pawn "Pawn" "P" tokin #f 0 0 ((shortmove 2)))
(bishop "Bishop" "B" horse #f 1 0 ((longmove 1285)))
(rook "Rook" "R" dragon #f 2 0 ((longmove 594)))
(lance "Lance" "L" prom-lance #f 3 0 ((longmove 2)))
(silver "Silver" "S" prom-silver #f 4 0 ((shortmove 1287)))
(gold "Gold" "G" #f #f 5 0 ((shortmove 599)))
(knight "Knight" "N" prom-knight #f 6 0 ((knightmove 2)))
(king "King" "K" #f #f 7 0 ((shortmove 1911)))
(tokin "Tokin" "+P" #f pawn 8 0 ((shortmove 599)))
(horse "Horse" "+B" #f bishop 9 0 ((longmove 1285) (shortmove 594)))
(dragon "Dragon" "+R" #f rook 10 0 ((longmove 594) (shortmove 1285)))
(prom-lance "Promoted Lance" "+L" #f lance 11 0 ((shortmove 599)))
(prom-knight "Promoted Knight" "+N" #f knight 12 0 ((shortmove 599)))
(prom-silver "Promoted Silver" "+S" #f silver 13 0 ((shortmove 599)))
)