#!/usr/bin/env ruby

krad_binaries = ["krad_radio", "krad_radio_ap", "krad_radio_vp", "krad_radio_daemon"]
krad_binaries_path = "/usr/local/bin"
krad_clientlib = "libkradradio_client.dylib"
krad_clientlib_path = "/usr/local/lib"
krad_appfolder_path = "/Users/oneman/kode/Krad\\ Radio.app"
krad_appfolder_exec_path = "#{krad_appfolder_path}/Contents/MacOS/"
krad_dep_libs = ["libcairo.2.dylib", "libxmmsclient.dylib", "libz.1.dylib", "libsamplerate.0.dylib"]
krad_dep_libs += ["libx264.119.dylib", "libtheora.0.dylib", "libogg.0.dylib", "libtheoraenc.1.dylib"]
krad_dep_libs += ["libtheoradec.1.dylib", "libvorbis.0.dylib", "libvorbisenc.2.dylib", "libopus.0.dylib"]
krad_dep_libs += ["libFLAC.8.dylib", "libswscale.2.dylib", "libavutil.51.dylib"]
krad_dep_libs_path = "/opt/local/lib"

krad_dep_deps = ["libSDL-1.2.0.dylib", "libxvidcore.4.dylib", "libspeex.1.dylib", "libschroedinger-1.0.0.dylib"]
krad_dep_deps += ["libopenjpeg.1.dylib", "libmp3lame.0.dylib", "libmodplug.1.dylib", "libbz2.1.0.dylib", "libpixman-1.0.dylib"]
krad_dep_deps += ["libpng15.15.dylib", "libxcb-shm.0.dylib", "libX11-xcb.1.dylib", "libxcb-render.0.dylib"]
krad_dep_deps += ["libXrandr.2.dylib", "libXext.6.dylib", "libXrender.1.dylib", "libX11.6.dylib"]
krad_dep_deps += ["libxcb.1.dylib", "libXau.6.dylib", "libXdmcp.6.dylib", "libiconv.2.dylib"]
krad_dep_deps += ["libfontconfig.1.dylib", "libfreetype.6.dylib", "liborc-0.4.0.dylib", "libexpat.1.dylib"]


for file in krad_binaries
  `cp #{krad_binaries_path}/#{file} #{krad_appfolder_exec_path}`
  for depfile in krad_dep_libs
    `install_name_tool -change #{krad_dep_libs_path}/#{depfile} @executable_path/#{depfile} #{krad_appfolder_exec_path}#{file}`
  end
  `install_name_tool -change /Users/oneman/kode/krad_radio/.waf_build_directory/clients/libkradradio_client.dylib @executable_path/libkradradio_client.dylib #{krad_appfolder_exec_path}#{file}`
end

`cp #{krad_clientlib_path}/#{krad_clientlib} #{krad_appfolder_exec_path}`
`install_name_tool -id @executable_path/#{krad_clientlib} #{krad_appfolder_exec_path}#{krad_clientlib}`
for depfile in krad_dep_libs
  `install_name_tool -change #{krad_dep_libs_path}/#{depfile} @executable_path/#{depfile} #{krad_appfolder_exec_path}#{krad_clientlib}`
end


for file in krad_dep_libs
  `cp #{krad_dep_libs_path}/#{file} #{krad_appfolder_exec_path}`
  `install_name_tool -id @executable_path/#{file} #{krad_appfolder_exec_path}/#{file}`
  for depfile in krad_dep_libs
    `install_name_tool -change #{krad_dep_libs_path}/#{depfile} @executable_path/#{depfile} #{krad_appfolder_exec_path}#{file}`
  end
  for depfile in krad_dep_deps
    `install_name_tool -change #{krad_dep_libs_path}/#{depfile} @executable_path/#{depfile} #{krad_appfolder_exec_path}#{file}`
  end
end

for file in krad_dep_deps
  `cp #{krad_dep_libs_path}/#{file} #{krad_appfolder_exec_path}`
  `install_name_tool -id @executable_path/#{file} #{krad_appfolder_exec_path}/#{file}`
  for depfile in krad_dep_libs
    `install_name_tool -change #{krad_dep_libs_path}/#{depfile} @executable_path/#{depfile} #{krad_appfolder_exec_path}#{file}`
  end
  for depfile in krad_dep_deps
    `install_name_tool -change #{krad_dep_libs_path}/#{depfile} @executable_path/#{depfile} #{krad_appfolder_exec_path}#{file}`
  end
end

# install_name_tool -id @executable_path/../Frameworks/Jackmp.framework/Jackmp Jackmp

`install_name_tool -change /Library/Frameworks/Jackmp.framework/Versions/A/Jackmp @executable_path/../Frameworks/Jackmp.framework/Versions/A/Jackmp #{krad_appfolder_exec_path}krad_radio_daemon`
`install_name_tool -change /Library/Frameworks/Jackmp.framework/Versions/A/Jackmp @executable_path/../Frameworks/Jackmp.framework/Versions/A/Jackmp #{krad_appfolder_exec_path}libavutil.51.dylib`
`install_name_tool -change /Library/Frameworks/Jackmp.framework/Versions/A/Jackmp @executable_path/../Frameworks/Jackmp.framework/Versions/A/Jackmp #{krad_appfolder_exec_path}libswscale.2.dylib`

`rm /Users/oneman/kode/Krad\\ Radio\\ 12\\ Mac.tar.bz2`
`tar cjvf /Users/oneman/kode/Krad\\ Radio\\ 12\\ Mac.tar.bz2 -C /Users/oneman/kode Krad\\ Radio.app`
