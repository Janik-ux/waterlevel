# sendAndMeasure.py
# (c) Janik-ux 2024
# licensed under MIT
# 
# Simulate the measuring and data sending behaviour
# of ESP32 waterlevel Sensor, to evaluate the algorithm,
# which tries to approximate the function well,
# but with as little measuring effort as possible.

import numpy as np
import matplotlib.pyplot as plt

# settings
max_t = 36000 # s = 1h
norm_pegel = 300 # mm
max_anstieg = 1.5 # mm/s
max_sleep = 2000
h_res = 1 # mm Höhenauflösung

# init data vars
time = np.arange(max_t)
pegel = np.arange(max_t*2)
pegel_x = np.arange(0, max_t, 0.5)
messung = np.array([norm_pegel])
messung_x = np.array([0])
senden = np.array([])
senden_x = np.array([])

# create random pegel function
for i in pegel:
    if i==0:
        pegel[i] = norm_pegel
        continue
    proba = (-0.5/norm_pegel**3)*((pegel[i-1]-norm_pegel)**3)+0.5 # polynom 3. grades
    #print(pegel[i-1], proba)
    pegel[i] = pegel[i-1] + -1+2*np.random.binomial(1, proba)

# run pegel messer
t = 0
pegel_dic = {}
while t<max_t:
    # Messung im RTC speicher des ESP speichern
    pegel_dic[t] = pegel[t*2]

    # loggen, dass gemessen wurde (nur in Simulation nötig)
    messung = np.append(messung, pegel_dic[t])
    messung_x = np.append(messung_x, t)


    anstieg = (messung[-1]-messung[-2])/(messung_x[-2]-messung_x[-1])
    if anstieg == 0: anstieg = 0.0001
    
    # Daten senden, wenn notwendig
    if anstieg > max_anstieg or len(pegel_dic) > 10:
        senden = np.append(senden, len(pegel_dic))
        senden_x = np.append(senden_x, t)
        pegel_dic = {}

    
    sleep_time = (h_res/abs(anstieg)) if (h_res/anstieg) < max_sleep else max_sleep
    print(t, sleep_time)
    sleep_time = int(sleep_time)

    t+=sleep_time


print(messung)
print(senden)
plt.plot(pegel_x, pegel)
plt.plot(messung_x, messung)
plt.scatter(senden_x, senden, c="red")
plt.ylabel("Pegelstand")
plt.show()

