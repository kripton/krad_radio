dir = "/data2/youtube"

while true do
Dir.foreach(dir) do |item|
	next if item == '.' or item == '..'	
		if item.include? ".webm"
			puts dir + "/" + item
			puts `mkvinfo "#{dir}/#{item}" | grep Dura`.chomp
#			puts `mkvinfo #{dir}/#{item} | grep width`.chomp

			`krad_radio radio1 rm 1`
			sleep 1
			`krad_radio radio1 play "#{dir}/#{item}"`

			puts "PLAYING #{dir}/#{item}"			

			sleep 300

			puts "----\n"
		end
end
end
