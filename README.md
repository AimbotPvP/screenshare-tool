# screenshare-tool
### A minecraft screenshare tool in c++

This is a really simple screenshare tool made for you skids that just go through .txt files and look for strings there.

### Getting started
 - Install visual studio (if you don't already have a x86 compiler or visual studio installed)
 - Start visual studio and just import entrypoint.cc
 - Go to properties -> c/c++ -> precompiled headers -> precompiled header and set it to "Not Using Precompiled Headers" like this ![](https://im.killingmyself.today/USj9jYGQug.png)
 - Now you're probably going to want to add in your strings (line 136)
   ```cpp
   	const char *strings[] =
	{
		"Aim Assist",
    "Auto Clicker",
    "insane vape v3 detection"
	}; /// NOTE: these are case sensitive
   ```
  - Change the compile mode to x86 and build the solution (F7)

 If you followed every step then you should have a executable application
 
 ### Disclaimer
 
 This code isn't the best and definitely shouldn't be used for any commercial purposes
