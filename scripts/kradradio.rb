class KradStation

	def initialize (name_or_url)
		@name = name_or_url
	end


	def cmd (action)
		`krad_radio #{@name} #{action}`
	end

	def info ()
		return `krad_radio #{@name} info`.chomp
	end

	def uptime ()
		return `krad_radio #{@name} uptime`.chomp
	end
	
	def set_resolution (width, height)
		`krad_radio #{@name} res #{width} #{height}`
	end

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
				self.cmd (" rm 2")
				sleep 0.3
				self.cmd ("play \"#{playlist_dir}/#{item}\"")
				puts "PLAYING (duration #{duration}) #{playlist_dir}/#{item}"
				puts ""
				sleep duration
			end
			self.cmd (" rm 2")
		end
	end
  
end
