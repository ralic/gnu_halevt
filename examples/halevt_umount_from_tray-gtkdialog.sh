#! /bin/sh

device=$1
label=$2

if [ z"$device" = z ]; then
  export MAIN_DIALOG='
 <vbox>
  <text>
    <label>Error: missing device argument</label>
  </text>
  <hbox>
   <button ok></button>
  </hbox>
 </vbox>
'
  gtkdialog --program=MAIN_DIALOG
  exit 1
fi

[ z"$label" = 'z' ] && label=$device

TMPFILE=`mktemp /tmp/halevt_umount_gtkdialog.XXXXXX` || exit 1

export MAIN_DIALOG="
<window title=\"Umount $label\">
<vbox>
  <frame Umount $label>
  <text>
    <input file>$TMPFILE</input>
    <variable>REPORT</variable>
  </text>
  </frame>
  <hbox>
  <button>
    <label>Umount</label>
    <action>halevt_umount_report.sh $TMPFILE \"$label\"</action>
    <action type=\"refresh\">REPORT</action>
  </button>
  <button ok>
  </button>
  </hbox>
</vbox>
</window>
"

gtkdialog --program=MAIN_DIALOG
