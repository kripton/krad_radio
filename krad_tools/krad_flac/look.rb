#require 'find'
 
#Find.find('/data2') do |f|

dirs = ["/data2", "/data3", "/music", ENV['HOME']]

#dirs = ["/home/oneman/Downloads"]

found = 0
not_expected = 0
str = ""

for dir in dirs

	pattern = dir + "/**/*"
	str = ""
	
	Dir[pattern].each do |f|

		filename = f.to_s.chomp

		if filename[-5,5].downcase == ".flac"
			str = `./flac_info_test "#{filename}"`
			found += 1
			if str != ""
				not_expected += 1
				puts str
			end
		end

	end
	
end

puts "Found #{found} flac files, #{not_expected} non 44100k 2chan 16bit"
