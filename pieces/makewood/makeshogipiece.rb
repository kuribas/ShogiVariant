#!/usr/bin/env ruby
require 'socket'

class GimpClient
   attr_reader :error
   attr_reader :errormsg
   def initialize(server = 'localhost', port = 10008)
      open(server, port)
      @error = 0
      @errormsg = ""
   end

   def open(server = 'localhost', port = 10008)
      @client = TCPSocket.new(server, port)
   end
   
   def send(str)
      len = str.length
      @client << "G" << (len >> 8).chr << (len & 0xFF).chr << str
      
      return if @client.getc != ?G
      @error = @client.getc
      len = @client.getc
      len = (len << 8) + @client.getc
      @errormsg = ""
      len.times{ @errormsg += @client.getc.chr }
      return @errormsg
   end
  
   def close
      @client.close
   end
end

$gimpclient = GimpClient.new

$gimpscript = '(let* ((image (car (file-pnm-load 0 "$INFILE" "$INFILE")))
      (masklayer (car (gimp-image-get-active-layer image)))
      (bluredmask (car (gimp-layer-copy masklayer 0)))
      (woodback (car (gimp-layer-new image (car (gimp-image-width image))
				     (car (gimp-image-height image))
				     0 "woodback" 100 0))))
  (gimp-palette-set-background \'(0 0 0))
  (gimp-layer-resize bluredmask (+ (car (gimp-image-width image)) 40)
		     (+ (car (gimp-image-height image)) 40) 20 20)
  (gimp-convert-rgb image)
  (gimp-invert bluredmask)
  (plug-in-gauss-rle 1 image bluredmask
		     (/ (car (gimp-image-width image)) 10)
		     1 1)
  (gimp-layer-resize bluredmask (car (gimp-image-width image))
		     (car (gimp-image-height image)) -20 -20)
  (gimp-image-add-layer image woodback 0)
  (gimp-patterns-set-pattern "Pine?")
  (gimp-image-set-active-layer image woodback)
  (gimp-selection-all image)
  (gimp-bucket-fill woodback 2 0 100 0 0 0 0)
  (gimp-selection-none image)
  (plug-in-bump-map 1 image woodback bluredmask 180 45.0 2 0 0 0 0 1 0 0)
  (gimp-image-remove-layer image masklayer)
  (gimp-image-remove-layer image bluredmask)
  (file-pnm-save 1 image woodback "$OUTFILE" "$OUTFILE" 1)
  (gimp-image-delete image))'

class Pipe
   def self.send(str, command)
      Pipe.new(command).send(str)
   end

   def initialize(str)
      @command = str
   end

   def send(str)
      io = IO.popen(@command, "r+")
      io.write(str)
      io.close_write
      output = io.read
      io.close
      return output
   end
end

def createback(infile, outfile)
   script = $gimpscript.gsub("$INFILE", infile).gsub("$OUTFILE", outfile)

   begin
      $gimpclient.send(script)
      if $gimpclient.error != 0
         $stderr << "GimpServer Fout: " << $gimpclient.errormsg
      end
   rescue
      $stderr << "Fout: Kon niet schrijven naar gimpserver\n"
      return false
   end
end


$masks = {}
def make_mask(alpha)
   if $masks[alpha]
      File.open("back.ppm") do |f|
         f.write($masks[alpha])
      end
   else
      createback("alpha.pgm","back.ppm")
      $masks[File.read("alpha.pgm")] = File.read("back.ppm")
   end
end


ARGV.each do |f|
   input = f
   output = File.basename(input, ".png") + "_wood.png"

   ppmpiece = `pngtopnm #{f} | pnmcrop`
   width, height = Pipe.send(ppmpiece, 'pnmfile').
                   split.values(4, 6)

   print(input, " ")
   makered = Pipe.send(ppmpiece, 'ppmtomap 2>&1 > /dev/null').
             index('4 colors') ? true : false
   
   Pipe.send(ppmpiece, PNMSHOGI + " -t > textmask.ppm")
   points = Pipe.send(ppmpiece, OUTLSCAN).
            split.map{ |p| p.to_i + 20}
   alpha = `#{OUTLINE} #{width + 40} #{height + 40} #{points.join(" ")} | tee alpha.pgm`

   make_mask(alpha)
   
   if makered
      <<`EOL`
      pnmtile #{width} #{height} #{SHOGIPIECEDIR}/woodtextback.ppm |
      pnmcomp -invert -alpha=textmask.ppm back.ppm |
      ppmquant -map #{SHOGIPIECEDIR}/colormap.ppm |
      pnmtopng -alpha alpha.pgm 2> /dev/null > wood/#{output}
EOL
   else
      <<`EOL`
      ppmmake '#ca0d0d' #{width} #{height} |
      pnmcomp -invert -alpha=textmask.ppm back.ppm |
      ppmquant -map #{SHOGIPIECEDIR}/colormap.ppm |
      pnmtoxpm -alpha alpha.pgm 2> /dev/null > wood/#{output}
EOL
   end
   
   if $?.exitstatus != 0
      puts " ==> unresolved"
   else
      puts " ==> " + output
   end
end

$gimpclient.close
