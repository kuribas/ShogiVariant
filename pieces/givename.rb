#!/bin/ruby

if ARGV.length != 1
  $stderr.puts "Onjuist aantal parameters opgegeven"
  exit
end
pieces = Array.new
prom = Array.new
files = `ls -r *.png`.split
names = Hash.new()
n = 0
File.open(ARGV[0], "r") { |file| 
  3.times{ file.gets }
  file.each{ |line|
    data = line.split(",")
    name = data[1]
    break if n + 1 != data[0].to_i
    n += 1
    if not data[5].empty?
      prom[data[5].to_i] = n
    else
      names[name] = true
    end
    pieces.push(data)
  }
}
n = 0
pieces.each { |data|
  n += 1
  name = data[1]
  name = "P_" + name if prom[n] and names[name]
  name = name.gsub(" ","_") + "_B.png"
  next if names[name]
  names[name] = true
  fileno = data[6].to_i - 1
  `mv #{files[fileno]} #{name}`
}
