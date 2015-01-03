(use util.match)
(use file.util)

(define moves-rx #/BasicMove \*(\w*?)_?moves\[\]\s*=\s*{(.*?),\s*NULL}/)
(define piece-rx #/Piece (\w*)\s*=\s*{(.*?)}/)

(define (unreference str)
  (regexp-replace-all #/&/ str ""))

(define (schemify str)
  (and (not (equal? str "NULL"))
       (string->symbol
	(unreference (regexp-replace-all #/[_ ]/ str "-")))))

(define (remove-quotes str)
  (regexp-replace #/\s*"(.*?)"\s*/ str "\\1"))

(define (read-moves moves)
  (map
   (lambda (move)
     (match (string-split move #\_)
       ((name . args)
	(cons (schemify name)
	      (map (cut string->number <> 16) args)))))
   (string-split moves #/,\s*/)))

(define (read-attrs attrs)
  (match (string-split attrs #/,\s*/) 
    ((name abbrev promote-to promote-from num rank . _)
     (list (remove-quotes name) (remove-quotes abbrev)
	   (schemify promote-to) (schemify promote-from)
	   (string->number num) (string->number rank)))))
  
(define (extract-pieces str)
  (rxmatch-if (moves-rx str)
      (#f piece moves)
    (let ((match2 (piece-rx str)))
      (rxmatch-if match2
	  (#f piece2 attrs)
	(if (not (equal? piece piece2))
	    (error "Matching moves not found.")
	    (let ((moves (read-moves moves))
		  (attrs (read-attrs attrs)))
	      (cons (append (cons (schemify piece) attrs)
			    (list moves))
		    (extract-pieces (rxmatch-after match2)))))
	(error "Matching moves not found.")))
    '() ))

(define (read-pieces file)
  (extract-pieces (file->string file)))

(for-each
  (lambda (dir)
    (call-with-output-file (path-swap-extension dir "scm")
      (lambda (out)
	(display "(" out)
	(for-each
	    (lambda (p) (write p out) (newline out))
	    (extract-pieces (file->string dir)))
	(display ")" out))))
  (sys-glob "var*.C"))
