# Generate spawn points file (src/resources/SPAWNPOINTS.TXT)
# Spawn points should be in circular formation centered at -2.5, -30
# with a radius of 40. The spawn points should be evenly spaced
# The spawn points should be pointed towards the center of the map

# Format:
# x y z // x, y, and z coordinates of spawn point
# x y z // x, y, and z coordinates of forward vector

import math

# Number of spawn points
num = 12

# Radius of spawn points
radius = 22

# Center of spawn points
centerX = 3
centerY = 7.5

# Height of spawn points
height = 2


with open("./src/resources/SPAWNPOINTS.TXT", "w") as f:
    f.write(f"{num}\n")
    for i in range(num):
        # Generate spawn points
        angle = (2 * math.pi / num) * i
        x = centerX + radius * math.cos(angle)
        z = centerY + radius * math.sin(angle)
        # format floats to 2 decimal places
        f.write(f"{x:.2f} {height} {z:.2f}\n")
        
        # Generate forward vectors (point towards center of map)
        angle = (2 * math.pi / num) * i
        x = math.cos(angle)
        z = math.sin(angle)
        f.write(f"{x:.2f} 0 {z:.2f}\n")
    
    f.flush();

