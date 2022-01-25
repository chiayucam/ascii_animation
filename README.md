# ascii_animation
Turn videos into ascii animation in the Windows command console (prototype)  
Uses FFmpeg libraries to decode video into AVframes and display them as ascii chars in command line console

## Examples
1. Bad apple  
https://user-images.githubusercontent.com/48814609/151071355-0612967d-9d50-433f-abe3-dad3d319a8cf.mp4  
Source: https://www.youtube.com/watch?v=9lNZ_Rnr7Jc

2. Holymyth vs Ina  
https://user-images.githubusercontent.com/48814609/151070325-19a6fd48-686e-4239-992b-d86eb8bee2b5.mp4  
Source: https://www.youtube.com/watch?v=vWLBFdf_ss0


## To do list
* clean up and refactor messy cody
* add option to change display quality of resolution and adjust the fontsize accordingly
* automatically choose proper resolution ratio according to original video's aspect ratio
* add audio decoding and playback
* proper video frame playback timestamps (currently doesn't match original video timestamps)
