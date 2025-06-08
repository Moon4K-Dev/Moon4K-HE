# Adding Songs (from other Rhythm Games) to Moon4K

* Adding songs from other rhythm games to Moon4K is fairly easy due to it using [Tsukiyo](https://github.com/Moon4K-Dev/Tsukiyo)! A c++ library that helps manage the many different chart types for rhythm games!
* As of right now Moon4K currently supports these charts:

```
Friday Night Funkin' (LEGACY 0.2x) - TESTED (Since shit like Psych, KE, etc, etc is built on top of this they SHOULD work too!)
Friday Night Funkin' (V-SLICE) - UNTESTED (MIGHT HAVE TO WORK ON THIS AFTER I PUSH THIS COMMIT LOL)
StepMania - TESTED
osu!mania - TESTED
```

* Below contains the information on adding those types of charts to Moon4K.

## Adding FNF (Legacy 0.2x) charts to Moon4K

* Adding Friday Night Funkin's Legacy charts is as simple as grabbing the chart, the instrumental, and the voices file and dropping them all in ```YOUR_MOON4K_DIRECTORY/assets/charts/FNF_SONG_NAME``` (in lowercase)
* Make sure to rename Inst.ogg and Voices.ogg to ```FNF_SONG_NAME_Inst.ogg``` and ```FNF_SONG_NAME_Voices.ogg``` (in lowercase)
* After that the game should detect it and you're good to play it!

## Adding FNF (V-Slice) charts to Moon4K

* UNTESTED LOL

## Adding StepMania charts to Moon4K

* All you literally have to do is grab the stepmania chart and ogg file, make a folder in ```YOUR_MOON4K_DIRECTORY/assets/charts/``` and the game should detect it and you can play!


## Adding osu!mania charts to Moon4K

* Adding osu!mania charts is also very simple! 
* First open the .osz file in 7zip or whatever you use!
* Then grab the mp3 or ogg file and the .osu file that you want to use!
* Go ahead and convert the mp3 file to ogg if your .osz file has that
* Then make a folder in ```YOUR_MOON4K_DIRECTORY/assets/charts/``` with your song name and add everything there!
* The game should detect it and you're good to go!