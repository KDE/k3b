K3b Version 18.04.0

Thanx for downloading K3b - The CD Kreator

These are the features so far:
 - the most userfriendly interface ever ;-)
   - themable

 - Media-centric user interface:
   - Select the medium to use for burning instead of the device
   - K3b always knows about all optical devices and inserted media and adjusts the UI accordingly

 - writing audio-cds
   - On-the-fly decoding of many audio formats through plugin struxture
     (decoding plugins for mp3, ogg vorbis, flac, wave, musepack, wma, aiff, and others)
   - CD-Text support
   - Metadata support, fill CD-Text from metadata or via Internet queries (CDDB + Musicbrainz)
   - little gimmick: hide the first track (so that you have to search back from the 
     beginning of the cd to find it)
   - volume level normalization (only when writing with an image)
   - multiple sources for one track possible (merging of tracks, splitting of tracks, 
     adding silence of arbitrary length)
   - integrated "preview" player
   - Directly add audio tracks from other CDs and let K3b take care of the rest.
   
 - writing ISO9660-CDs and DVDs
   - Joliet/Rockridge support
   - Udf filestructures (no pure Udf so far)
   - create image/write image
   - writing on-the-fly
   - creating of file-tree via drag'n'drop (as easy as it could be)
   - removing files and directories from data tree
   - moving files within the project
   - adding new empty directories to data tree
   - renaming of files (manually or automatically for mp3-files) (for joliet and rockrigde)
   - support for most of the mkisofs-options (I don't think anyone will ever use them! ;-))
   - multisession support (including importing old sessions and overwriting files from old sessions)
     Automagically multisession handling: start, continue, or finish multisession CDs and DVDs based on 
     the size of the project and the remaining capacity on the media.
   - El Torito bootable CD/DVD support

 - writing Video CDs
   - VCD 1.1, 2.0, SVCD
   - CD-i support (Version 4)
   
 - writing mixed-mode CDs
   - CD-Extra (CD-Plus, Enhanced Audio CD) support

 - writing eMovix CDs and DVDs
   
 - writing data DVDs 
   - Support for DVD-R(W) and DVD+R(W)
   - Support for DVD+R Double Layer media

 - formatting DVD-RWs and DVD+RWs

 - writing existing iso-images and cue/bin images to CD
   - Writing of Audio cue file images on-the-fly (All audio formats supported)

 - CD copy (data, audio, mixed mode)

 - DVD copy

 - CD cloning with cdrecord >=2.01a17

 - blanking of CD-RWs
   - Also automagically before writing a CD.

 - CD ripping to wav, mp3, ogg-vorbis, flac, or whatever format needed
   - encoding plugin system
   - Cddb support
   - sophisticated pattern system to automatically organize the ripped tracks
   - m3u playlist creation
   - CD-TEXT support

 - DVD ripping with the transcode tools
   - Support for automatic clipping
   - DivX/XviD encoding

 - Retrieving CD/DVD info and toc

 - Powerful default and automatic settings

 
See INSTALL for further information


Please report bugs (with k3b output!) https://bugs.kde.org/ and tell me what
you like/dislike about the user-interface!

Have fun
Sebastian Trueg (trueg@k3b.org)
Leslie Zhai (zhaixiang@loongson.cn)
