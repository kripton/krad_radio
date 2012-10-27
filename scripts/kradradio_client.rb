
def get_image_dim(filename)

	if (filename.downcase.include? ".png")
		splitter = "PNG"
	else
		splitter = "JPEG"
	end

	dims = `identify \"#{filename}\"`.chomp.split(splitter)[1].split(" ")[0].split("x")
	width = dims.first.to_i
	height = dims.last.to_i
	
	puts "got #{width} #{height} for #{filename} - #{dims}"
	
	return width, height
end

def center_or_zero(max, item)

	if item > max
		return 0
	else
		return (max - item) / 2
	end

end

def place_image(filename, width, height)

	imagewidth,imageheight = get_image_dim(filename)
	
	center_width = center_or_zero(width, imagewidth)
	center_height = center_or_zero(height, imageheight)
	
	puts "got center of #{center_width}-#{center_height} for image #{imagewidth}x#{imageheight} for max of #{width}x#{height}"
	
	return center_width, center_height

end

class KradStation

	def initialize(name_or_url)
		@name = name_or_url
		@comp_width = 0
		@comp_height = 0
		Kernel.srand
    if (ARGV[0] == "restart")
      self.cmd("destroy")
    end
		self.launch_daemon
		sleep 0.666
	end

	def launch_daemon
		self.cmd("launch")
	end

	def sprite(filename, x=0, y=0, rate=5, scale=1, opacity=1.0, rotation=0.0)
		self.cmd("addsprite \"#{filename}\" #{x} #{y} #{rate} #{scale} #{opacity} #{rotation}")
	end
	
	def set_sprite(num=0, x=0, y=0, rate=5, scale=1, opacity=1.0, rotation=0.0)
		self.cmd("setsprite #{num} #{x} #{y} #{rate} #{scale} #{opacity} #{rotation}")
	end
	
	def rm_sprite(num=0)
		self.cmd("rmsprite #{num}")
	end
	
	def throw_sprite(num=0)
		if (rand(100) % 100 > 50)
			if (rand(100) % 100 > 50)
				if (rand(100) % 100 > 50)
					self.set_sprite(num, 0, 2333, 5, 0.3, 0, 545)
				else
					self.set_sprite(num, 0, -1333, 5, 3.3, 0, -545)
				end
			else
				if (rand(100) % 100 > 50)
					self.set_sprite(num, 1333, 0, 5, 2.3, 0, 45)
				else
					self.set_sprite(num, -1333, 0, 5, 0.1, 0, -45)
				end
			end
		else
			if (rand(100) % 100 > 50)
				if (rand(100) % 100 > 50)
					self.set_sprite(num, 1333, 1333, 5, 0.3, 0, 45)
				else
					self.set_sprite(num, -1333, -1333, 5, 0.8, 0, -45)
				end
			else
				if (rand(100) % 100 > 50)
					self.set_sprite(num, -1333, 0, 45, 0.5, 0, 15)
				else
					self.set_sprite(num, 0, -1333, 15, 0.5, 0, -45)
				end
			end
		end
	end

	def text(thetext, x=0, y=0, rate=5, scale=32, opacity=1.0, rotation=0.0, red=0, green=0, blue=0, font="sans")
		self.cmd("addtext '#{thetext.gsub(/[^0-9a-z :@]/i, '')}' #{x} #{y} #{rate} #{scale} #{opacity} #{rotation} #{red} #{green} #{blue} #{font}")
	end
	
	def set_text(num=0, x=0, y=0, rate=5, scale=32, opacity=1.0, rotation=0.0, red=0, green=0, blue=0)
		self.cmd("settext #{num} #{x} #{y} #{rate} #{scale} #{opacity} #{rotation} #{red} #{green} #{blue}")
	end
	
	def rm_text(num=0)
		self.cmd("rmtext #{num}")
	end

	def record(filename, options={})

		width = ""
		height = ""
		bitrate = ""
		codec = ""
		audiobitrate = ""

		if options.has_key?(:codec)
			codec = options[:codec]
		else
			codec = "default"
		end

		if options.has_key?(:width)
			width = options[:width]
		end

		if options.has_key?(:height)
			height = options[:height]
		end

		if options.has_key?(:bitrate)
			bitrate = options[:bitrate]
		end

		if options.has_key?(:audiobitrate)
			audiobitrate = options[:audiobitrate]
		end

		self.cmd("record audiovideo #{filename} #{codec} #{width} #{height} #{bitrate} #{audiobitrate}")
	end

	def record_audio(filename, options={})

		codec = ""
		audiobitrate = ""

		if options.has_key?(:codec)
			codec = options[:codec]
		else
			codec = "default"
		end

		if options.has_key?(:bitrate)
			audiobitrate = options[:bitrate]
		end

		if options.has_key?(:audiobitrate)
			audiobitrate = options[:audiobitrate]
		end

		self.cmd("record audio #{filename} #{codec} #{audiobitrate}")
	end

	def record_video(filename, options={})

		width = ""
		height = ""
		bitrate = ""
		codec = ""

		if options.has_key?(:codec)
			codec = options[:codec]
		else
			codec = "vp8vorbis"
		end

		if options.has_key?(:width)
			width = options[:width]
		end

		if options.has_key?(:height)
			height = options[:height]
		end

		if options.has_key?(:bitrate)
			bitrate = options[:bitrate]
		end

		self.cmd("record video #{filename} #{codec} #{width} #{height} #{bitrate}")
	end

	def cmd(action)
		thecmd = "krad_radio #{@name} #{action}"
		puts "command: #{thecmd}"
		ret = `#{thecmd}`.chomp
		sleep 0.05
		return ret
	end

	def jack?
		return jacked?
	end	

	def jack_running?
		return jacked?
	end	
	
	def jacked?
		resp = `krad_radio #{@name} jacked`.chomp
		if resp.downcase == "yes"
			return true
		else 
			return false
		end
	end
	
	def get_v4l2_device(num=0)
		resp = `krad_radio #{@name} lsv`.chomp
		count = 0
		resp.lines do |device| 
			if (count == num)
				return device
			end
			count = count + 1
		end
		return ""
	end
	
	def get_v4l2_devices()
		return `krad_radio #{@name} lsv`.chomp
	end	

	def info()
		return `krad_radio #{@name} info`.chomp
	end

	def uptime()
		return `krad_radio #{@name} uptime`.chomp
	end
	
	def set_resolution(width, height)
		@comp_width = width
		@comp_height = height
		`krad_radio #{@name} res #{@comp_width} #{@comp_height}`
	end
	
	def play(filename)
		self.cmd("play \"#{filename}\"")
	end
	
	def slideshow(pictures_dir, timeper=3, label=true)
	
		Kernel.srand

			playlist = []
			count = 0
			Dir.foreach(pictures_dir) do |item|
				next if item == '.' or item == '..'	
				if (item.include? ".png" or item.include? ".PNG" or item.include? ".jpg" or item.include? ".JPG")
					playlist.push item					
				end
			end
			
			playlist.shuffle!			
			
			puts "Slideshow of #{playlist.length} pictures"
	
			playlist.each do |item|
				filename = pictures_dir + "/" + item
				xy = place_image(filename, @comp_width, @comp_height)
				x = xy.first
				y = xy.last
				self.sprite(filename, x, y)
				if item.include? "_frames_"
					if (count == 0) or (count == 1)
						self.set_sprite(1, 955, 255, 4, 3.3, 0.85, 25)
					end
					if (count == 2)
						self.set_sprite(0, 450, 335, 4, 3.3, 1, -25)
					end	
				end
				if (count == 1)
					self.throw_sprite(0)
				end
				if (count == 2)
					self.throw_sprite(1)
				end
				if (label == true)
					self.text(item, 35, 155, 5, 44, 1, 0, 255, 255, 55, "DroidSans")
					sleep 1.5
					self.set_text(0, 35, 155, 5, 44, 0, 0, 255, 255, 2)
					sleep 1.5
					self.rm_text(0)
					sleep timeper - 1
				else
					sleep timeper
				end
				if (count == 1)
					self.rm_sprite(0)
				end
				if (count == 2)
					self.rm_sprite(1)
					count = 0
				end		
				sleep 0.2
				count = count + 1
				self.rm_sprite(2)
				self.rm_sprite(3)
				sleep 0.2				
			end
			self.rm_sprite(0)
			self.rm_sprite(1)
	
	end

	def play_dir(playlist_dir)
	
		while true do

			Kernel.srand	

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
				self.play (playlist_dir + "/" + item)
				puts "PLAYING (duration #{duration}) #{playlist_dir}/#{item}"
				puts ""
				sleep duration
			end
			self.cmd (" rm 2")
		end
	end

	def play_dir_single(playlist_dir)

		Kernel.srand	

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
			self.cmd (" rm 1")
			sleep 0.3
			self.play (playlist_dir + "/" + item)
			puts "PLAYING (duration #{duration}) #{playlist_dir}/#{item}"
			puts ""
			sleep duration
			break
		end
		self.cmd (" rm 1")

	end
  
end
