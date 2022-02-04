# ascii_animation
Turn videos into ascii animation in the Windows command console (prototype)  
Uses FFmpeg libraries to decode video into AVframes and display them as ascii chars

## Usage
    ascii_animation.exe [-r {high|medium|low}] <video_path>

## Examples
1. Bad apple  


https://user-images.githubusercontent.com/48814609/151072052-bcaf106b-2133-4d8d-b470-ce08edeea4b5.mp4  


Source: https://www.youtube.com/watch?v=9lNZ_Rnr7Jc

2. Holymyth vs Ina  


https://user-images.githubusercontent.com/48814609/151071998-3c9b42f5-3f57-4de6-a0f3-d62675dfd56c.mp4  


Source: https://www.youtube.com/watch?v=vWLBFdf_ss0


3. Star wars IV opening


https://user-images.githubusercontent.com/48814609/151312670-c731b752-587f-4d16-9d8f-d4fd5bf54798.mp4


Source: https://www.youtube.com/watch?v=vLgsf8Pei6Q

## To do list

- [x] add option to change resolution quality and adjust the console fontsize accordingly
- [x] automatically choose proper resolution ratio according to original video's aspect ratio
- [ ] add audio decoding and playback
- [ ] proper video frame playback timestamps (currently doesn't match original video timestamps)
- [ ] clean up and refactor messy code
