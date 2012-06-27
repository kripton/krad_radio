def play_dir (station_sysname, playlist_dir)
	
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
			`krad_radio #{station_sysname} rm 2`
			sleep 0.3
			`krad_radio #{station_sysname} play "#{playlist_dir}/#{item}"`
			puts "PLAYING (duration #{duration}) #{playlist_dir}/#{item}"
			puts ""
			sleep duration
		end
		`krad_radio #{station_sysname} rm 2`
	end
end
