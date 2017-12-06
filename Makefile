all:
	gcc akai-audio.c -shared -fPIC -lopenal -o akai-audio.so
	rm ~/.weechat/plugins/akai-audio.so
	ln ./akai-audio.so ~/.weechat/plugins/akai-audio.so
