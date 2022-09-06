bob ombs turn into bomb ombs
big mario
high gravity
buttons swapped
level reset
things spawn goombas when they die (except for dust)
textures turn into toadfaces
collecting coins subtracts coins for a bit
tornado (all levels, fixed the old code)
mario says random shit for a while
mario choir music
screen is slightly rotated
marios visual position gets offset by a random amount
attempting to BLJ gives you PU speed instantly
bonking a wall without wallkicking explodes mario
star models swap
low gravity
mario randomly "trips" (when walking, you have a chance to just fall on your stomach
bouncing off an enemy shoots mario into space)
randomly swap caps
health drain
coins do damage instead
star collect fanfar is toad fanfare instead
collission types get changed to a random other one. make sure not to pick painting types. the swap is consistent between floors 
stars run away from mario 
quicktime event. press a certain button within 2 seconds or you die instantly. 
when you pause, the cursor moves to exit course 
mario starts spinning 
race enemies are HAULING ASS 
backwards push like holding bob ombs hand free 
longjump makes you GP in place 
spawn green demon  
very sleepy mario
health bar becomes invisible 
mario becomes invisible 
mario loses his cap in the current level(just knock mario over and he flies backwardsd) 
doors just knock you back and deal damage 
doors rotate mario 180 degrees, putting him back where he came from 
forwards walking speed gets uncapped and mario accelerates faster 
fall damage requires only 200 height. everything can give you fall damage now. 
every object rotates  
slow music
fast music
if you walk, your speed is instantly set to 48 if its below 48 
8 directional controls 
jumping warps mario up by 1000 coordinates but halfs his jumping v speed 
pressing B sets mario on fire 
remove the water in the level 
Z button is held 
pause is pressed every other frame  
camera buttons are pressed at random (C, R) 
wallkicks dont change marios angle 
marios anglevel is changed to 0x100, slowly turn with swim and wing cap 
enable vertex colors on the entire level
marios animations read from the wrong spot, cartridge tilt
ground pound automatically if your v speed is negative
sliding puts you on yout tum tum
jumping sticks mario to the ceiling and the ceiling becomes hangable
beta triple jump
if mario is in a speakable state, make a textbox appear
controls are inverted if coin count is an odd number 
pausing kills mario 
text becomes rainbow 
pressing z squishes mario
one ups knock mario out with an explosion dealing 2 dmg
everythign deals double damage
shell mario forwards push
coins count double
every surface is non slippery (0x15)
climbing upwarps mario
all hitboxes extend upwards infinitely
you can no longer hold forwards
minimum speed is set to 48 
wrong warps (level is randomized, always 0x0A) 
long jump bonks you backwards
game skips marios movement function at random or does it twice(stuttery movement)
death planes become normal surfaces 
mario gets a random color palette
mario turns into a signpost
mario gets a random HP value every frame
jump kicks give you the sm64land catdive
camera stops
pong appears on the screen. if you lose the game of pong, mario gets knocked over and takes 8 damage, if you win it disappears
drunk lakitu (fuck with some cam values at random)
randomly the level has the wrong BGM
wing cap sets marios speed to 200
mario repeats his sound over and over
everything gets set to a random scale
marios walk sounds become rabbit noises
debug fly mode for 5 seconds
when in spinjump, the nearest object also rotates around the y axis
WATER SURFACE ACAMERA
aglab cam
when mario takes damage, he loses 20 coins and drops them around him
lose a live 
everything always faces the camera
music is only on the left ear
lanky mario (scale him up on y axis, slowly, until the code runs out, hitbox too)
hyperaggressive boos (also go faster)
mario instantly turns to intended yaw



TODO:
time limit a lot of bad codes
	- high gravity
	- coin subtraction
	- tornado
make backwards push MUCH weaker
make forwards push MUCH weaker
remove bad codes?

new code ideas:
mess with some matrix math for visual effects



kaze:
make an array at 803f0000 that holds 4 byte timers for every effect, sorted by ID
make an array that holds prior timers at 803f0400
the effects that trigger on activation do stuff once the new timer is bigger than the old timer

melon:
make the streamer able to put a "base chaos frequency". this has to be put in seconds. recommended number: 5-10 (higher number: easier). write this number to 803F0800
make the streamer able to put a "base chaos duration". this has to be put in seconds. recommended number: 30. write this number to 803F0804

add (effecttime(in seconds) multiplied by 30) to the number at 803F0000 + codeID*4
just write 30 for the effects that have no timer