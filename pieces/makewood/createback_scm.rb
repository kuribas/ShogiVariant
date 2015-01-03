#!/usr/bin/env ruby
require 'socket'

if ARGV.length != 2
  $stderr << "Gebruik: createback_scm.rb bestandin bestanduit\n"
  exit
end

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

$gimpscript.gsub!("$INFILE", File.expand_path(ARGV[0]))
$gimpscript.gsub!("$OUTFILE", File.expand_path(ARGV[1]))

begin
  gc = GimpClient.new
  gc.send $gimpscript
  if gc.error != 0
    $stderr << "GimpServer Fout: " << gc.errormsg
  end
  gc.close
rescue
  $stderr << "Fout: Kon niet schrijven naar gimpserver\n"
end

