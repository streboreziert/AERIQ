## Idea description.

Our creation will be an air quality monitor that measures CO2 levels, humidity, tempereature and air pressure.

We first start by creating a PCB scheme in Ki Cad. We  do that by combining the following parts:
Sensor (temperature, pressure and humidity) - BME280;
Sensor (CO2) - SCD41-D-R2;
Screen - E2290KS0F1;
Microprocessor - ESP32-C3-MINI-1U-N4X;
Boost converter - TPS61291DRVT;

After that, we make a PCB design in Ki Cad following the previously developed scheme. Once it is ready we order it as well as the other parts. When they arrive we weld the parts to the PCB board and wire the resulting product to two AA bateries. 

The whole setup will sit within a plasic 3D printed case developed in Fusion by us.

AERIQ has wi-fi and it will send data through it to a server that will have a database which will hold it. We will develop a website that will show the users of AERIQ the data in a simplified and intuitive format using API.

Currently the idea is that the monitor will sit attatched to a wall just like other similar products.


## The Problem We Are Solving

As urbanisation accelerates and large-scale buildings increasingly dominate both cities and rural environments, people are spending unprecedented amounts of time indoors—often in densely occupied spaces. Under these conditions, indoor air quality deteriorates rapidly. Continuous human presence leads to elevated carbon dioxide (CO₂) concentrations and excessive humidity, two invisible yet critical threats to comfort, health, and productivity.

High CO₂ levels are strongly linked to drowsiness, fatigue, reduced concentration, and impaired cognitive performance. At levels commonly reached in everyday indoor environments, headaches and dizziness can occur, silently undermining well-being and efficiency. Excess humidity further degrades indoor conditions by increasing perspiration and discomfort, producing musty odours, and—over extended periods—encouraging mould growth and dust mite proliferation, both of which pose serious respiratory risks.

By continuously tracking CO₂ concentration and humidity, AERIQ notifies occupants exactly when ventilation becomes essential, enabling timely intervention. This leads to healthier indoor environments, improved alertness, and long-term protection of the building itself. Such devices are rarely inexpensive (Aranet 4 ≈ €200; Airvalent Premium ≈ €160), yet AERIQ performs not far behind industry leaders while costing just €80, including profit margins and delivery.

In addition, AERIQ integrates precise temperature and air-pressure sensors. Temperature monitoring helps maintain thermal stability—particularly in winter—reducing unnecessary heating losses and improving energy efficiency. Air-pressure data provides valuable insight into recurring symptoms such as migraines, dizziness, and headaches, which are often misattributed to other health issues. By correlating environmental conditions with physical well-being, AERIQ empowers users with clarity rather than guesswork.

## Potential Clients

Air quality fluctuates most dramatically in high-occupancy environments such as offices, university buildings, hotels, gyms, classrooms, and supermarkets—making these institutions prime adopters of AERIQ. Improved indoor air directly translates to higher productivity, better learning outcomes, enhanced customer comfort, and reduced sick leave.

Beyond commercial spaces, AERIQ is highly valuable in private households. Families can rely on objective temperature data to eliminate everyday disputes, allergy sufferers can maintain optimal humidity levels, and homeowners can prevent long-term structural damage caused by moisture. Property managers, landlords, and real-estate developers can also leverage AERIQ to ensure compliance with indoor air standards and increase property value.
