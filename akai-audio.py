SCR_NAME    = "akai-audio"
SCR_AUTHOR  = "Gavin Stark <gstark31897@gmail.com>"
SCR_VERSION = "0.1.0"
SCR_LICENSE = "GPL3"
SCR_DESC    = "Plays encoded audio submited in IRC channels"
SCR_COMMAND = "akai-audio"

try:
   import weechat
except:
   print "Script must be run under weechat. http://www.weechat.org"
import_ok = False

def fn_modify(data, modifier, modifier_data, string):
    weechat.prnt("", "got a modifier")
    return string

if __name__ == "__main__":
    if weechat.register(SCR_NAME, SCR_AUTHOR, SCR_VERSION, SCR_LICENSE, SCR_DESC, "", ""):
        weechat.hook_modifier("weechat_print", "fn_modify", "")
        weechat.prnt("", "Hello, from python script!")
