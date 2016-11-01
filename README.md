# Avara

## Avara Game Source Code
### Originally published in 1996 for MacOS.

With permission from Andrew Welch at Ambrosia Software, the Avara source code is now available under the MIT license. The idea of releasing Avara as open source has existed for a very long time, but the trigger to actually get it done was the 20th anniversary and the events around that. There's an excellent twitch.tv stream recording showing what the game looks like and there's an interesting podcast on Avara that was done shortly after the stream:

https://www.twitch.tv/scarletswordfish/v/89966091  
http://simplebeep.com/podcast/e47-avara  

### What's Included

The source code is as complete as I can make it, meaning a few files were left out because they were originally Apple PPCToolbox sample code. Some algorithms and potentially some lines of code are from the excellent Graphics Gems books - please contact me if there's an issue with having that code here. The rest of the code, as far as I can remember and tell from looking at it now, was written by me.

Hopefully some people will find this source code interesting. Some of it is probably over 25 years old already. I used a very simple subset of C++, originally made popular in the Think/Lightspeed C compiler (later known as Symantec C++, which is what was used to compile the 68K architecture code). 

I'm not providing the resource file, as you can essentially obtain that by downloading a copy of Avara. As far as I know, all my DXF models are included and it's possible there are a few extra ones as well that were not used in the game. Similarly, all of the sound sample files are included and I'm pretty sure they are uncompressed 8 bit sample files, so they should be quite easy to use even after all this time.

### Porting to New Platforms

I think especially the core game code is rather cleanly written on top of relatively simple libraries, so if there's any interest in porting the game to a modern platform, I would probably only use the game UI code that uses classic MacOS APIs as a reference to figure out how something worked and not try to make it work on any current OS.

One big decision you have to make is to either stick to fixed point math and provide your own libraries for that or to convert to floating point. If you do the latter, it is very important for the floating point math results to match exactly (to the last bit) on every client running the game. This is due to the way the serverless networking system works in Avara.

The source code files use MacOS Roman encoding, but aside from the copyright symbol, pretty much everything could be considered 7 bit ASCII. Line endings were recently changed to linefeed from the original Mac carriage returns.

### Afterword

Avara was meant to be just a quick LAN-based tank shooter game and I wanted to then move on to some more interesting games. I had a whole bunch of ideas and even some code for a game or two, but other than some university-related projects, I haven't worked in the games industry since this. Game design and programming have always been my calling. I was very slow to graduate and in 2010, I finally got my M.Sc. degree in computer science with a major in interactive digital media and a minor in game design. Somehow though, I have never really connected with the Finnish games industry, so I have worked on "more mundane" projects.

I started playing World of Warcraft in 2006 and I think studying their design and understanding theorycrafting kept the game designer in me entertained for years.

Comments and questions are welcome, but I cannot promise to answer every question. I will do my best to help with potentially missing files or other gaps in this repository.

Sincerely,

 Juri Munkki  
 Designer & software engineer of Avara  
 jmunkki@cameraid.com  
