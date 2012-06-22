def play_dir (playlist_dir)
	
	while true do
	
		playlist = []
		total_duration = 0
		total_duration_minutes = 0
	
		Dir.foreach(playlist_dir) do |item|
			next if item == '.' or item == '..'	
				if item.include? ".webm"
					playlist.push item
					info = `mkvinfo "#{playlist_dir}/#{item}" | grep Dura`.chomp
					total_duration += info.split(" ")[3].to_i					
				end
		end
		
		total_duration_minutes = total_duration / 60
		
		playlist.shuffle!
		
		puts "Playlist of #{playlist.length} items, duration of #{total_duration_minutes} minutes"
	
		playlist.each do |item|
			info = `mkvinfo "#{playlist_dir}/#{item}" | grep Dura`.chomp
			duration = info.split(" ")[3].to_i
			`krad_radio radio1 rm 3`
			sleep 0.3
			`krad_radio radio1 play "#{playlist_dir}/#{item}"`
			puts "PLAYING (duration #{duration}) #{playlist_dir}/#{item}"
			puts ""
			sleep duration
		end
	end
end
