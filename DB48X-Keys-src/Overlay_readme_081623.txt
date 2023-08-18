The provided SVG files can be used to generate PNG files suitable for use on a Cricut cutting machine.

I am no longer maintaining the MAC version of the SVG file - only the WINDOWS version.  The fonts are too finicky between the two platforms so I am standardizing on the WINDOWS version.

Fonts Required:
- Baskerville (for a few of the function labels (like x-root-of-y)
- DIN Condensed (majority of the text on the label)
- Snell Roundhand (for the script logo text)
- Printed Circuit Board (for the db48x logo)  https://www.dafont.com/printed-circuit-board.font

Some of the fonts required are Adobe fonts, and were already available on my Mac.  I had to export them from the Mac to Windows to be able
to use them on those PCs.  I cannot share them here - they must be obtained on your own.

NOTE: The tabbed overlay is not always going to work with the DM42 due to variation in the size of the tab slots on the calculator, so consider
	this tabbed version a "YMMV" solution.  The adhesive option should always work.

Premade PNG files are available in this repo and were created grom the WINDOWS SVG file.  These are named with the date appended to 
	the ends of the filenames in MMDDYY format.  I recommend using the newest files available. If you feel the need to make any tweaks,
	the source SVG files are provided.

The SVG files provided can be used to produce BOTH the tabbed overlays and adhesive overlays.  To generate the PNG files:
 - Tabbed Overlay***:
	- Make sure that the "Tabs" layer is visible
	- Make sure that the "Key Front - In Place Adhesive" layer is hidden
	- Make sure that the "Key Front - Standalone" layer is hidden
	- Select the entire graphic, including the tabs
	- Use the export tool in Inkscape, and select the "Export Selected Only" tickbox
	- I often find it helps to select the Selection tab in Inkscape next, and then toggle back to the Custom tab
	- Set the DPI to 1200
	- Confirm that the dimensions of your export are 72.46mm W x 89.069mm H
	- Export to the file of your choosing as a PNG file
 - Adhesive Overlay:
	- Make sure that the "Tabs" layer is hidden
	- Make sure that the "Key Front - In Place Adhesive" layer is visible
	- Select the entire graphic
	- Use the export tool in Inkscape, and select the "Export Selected Only" tickbox
	- I often find it helps to select the Selection tab in Inkscape next, and then toggle back to the Custom tab
	- Confirm that the dimensions of your export are 70.083mm W x 89.069mm H
	- Set the DPI to 1200
	- Export to the file of your choosing as a PNG file

*** If you are making a tabbed overlay, and you want the adhesive labels for the 7 keys that differ from the stock DM42 KB:
	- Hide ALL layers except for "Key Front - In Place Adhesive"
	- Select the entire set of visible key labels
	- Use the export tool in Inkscape, and select the "Export Selected Only" tickbox
	- I often find it helps to select the Selection tab in Inkscape next, and then toggle back to the Custom tab
	- Set the DPI to 1200
	- Confirm that the dimensions of your export are 21.75mm W x 59.75mm H
	- Export to the file of your choosing as a PNG file

If you are a Cricut user and want to use Cricut DesignSpace to make the cuts:
	- Use the Upload tool to import the PNG files into DesignSpace, selecting the "Complex" option
	- At the end of the process, choose the "Print then Cut" image option
	- Add the newly uploaded image to the Canvas
	- Rescale the size of the image on the Canvas to match the dimensions of the original file (note that DesignSpace
		uses cm for the scale, so adjust accordingly from mm).
	- Make it!
	- Click the Send to Printer button, and make sure that the "Add Bleed" and "Use System Dialog" are selected.
	- Print it on either self-adhesive printable vinyl or appropriate paper stock
	- After printing, apply lamination sheet to the front of the printed document
	- For the tabbed overlay, I also apply another lamination sheet to the back for better durability
	- Follow normal process for loading into the machine and cutting


DB48X_tabbed_nokeys_081623.png:
Dims:  72.46mm W x 89.069mm H

DB48X_overlay_nokeys_081623.png:
Dims: 70.083mm W x 89.069mm H

DB48X_tabbed_withkeys_081623.png:
Dims:  72.46mm W x 89.069mm H

DB48X_overlay_withkeys_081623.png:
Dims: 70.083mm W x 89.069mm H

DB48X_keysonly_081623.png:
Dims: 26.25mm W x 18.10mm H

