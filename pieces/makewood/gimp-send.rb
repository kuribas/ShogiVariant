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
