/***********************************
   Copyright 2013 Ravishankar Mathur

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
***********************************/

#include <OpenFrames/WindowProxy>
#include <OpenFrames/FrameManager>
#include <OpenFrames/Sphere>
#include <OpenFrames/LatLonGrid>
#include <OpenFrames/RadialPlane>
#include <iostream>

#include <osgDB/ReadFile>

using namespace OpenFrames;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main()
{
	double r_earth = 6378.137;
	double r_sun = 695990.0;
	double AU = 149597900.0; // [km], Mean Earth-Sun distance

	  // Create the interface that will draw a scene onto a window.
	WindowProxy* theWindow = new WindowProxy(30, 30, 800, 600, 1, 1);
	theWindow->setID(0);

	  // Create the spheres that will populate the scene using
	  // Sphere(name, color[r,g,b,a])
	Sphere *earth = new Sphere("Earth", 0, 0, 1, 0.9);
	Sphere *sun = new Sphere("Sun", 1, 1, 0, 1.0);

	// Create the latitude/longitude grid on the earth
	LatLonGrid *earthgrid = new LatLonGrid("EarthGrid", 0, 0, 1, 1.0);
	earthgrid->showAxes(ReferenceFrame::NO_AXES);
	earthgrid->showAxesLabels(ReferenceFrame::NO_AXES);
	earthgrid->showNameLabel(false);

	// Create the equatorial plane
	RadialPlane *plane = new RadialPlane("Equator Plane");
	plane->setPlaneColor(1, 0, 0, 0.2);
	plane->setLineColor(1, 1, 1, 0.2);
	plane->showAxes(ReferenceFrame::NO_AXES);
	plane->showAxesLabels(ReferenceFrame::NO_AXES);
	plane->showNameLabel(false);

	// Create an inclined plane
	RadialPlane *plane2 = new RadialPlane("Inclined Plane");
	plane2->setPlaneColor(0, 1, 0, 0.2);
	plane2->setLineColor(1, 1, 1, 0.2);
	plane2->showAxes(ReferenceFrame::NO_AXES);
	plane2->showAxesLabels(ReferenceFrame::NO_AXES);
	plane2->showNameLabel(false);
	osg::Quat att;
	att.makeRotate(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.5, 0.5, 0.5));
	plane2->setAttitude(att[0], att[1], att[2], att[3]);
	plane2->setPosition(r_earth/1.73, r_earth/1.73, r_earth/1.73);

	  // Set texture
	earth->setTextureMap("../Images/EarthTexture.bmp");
	
	  // Set auto level of detailing
	earth->setAutoLOD(true);

	  // Set radii
	earth->setRadius(r_earth);
	earthgrid->setParameters(r_earth, M_PI/4.0, M_PI/3.0);
	plane->setParameters(10.0*r_earth, 2.0*r_earth, M_PI/6.0);
	plane2->setParameters(r_earth, 0.0, M_PI/6.0);
	sun->setRadius(r_sun);

	  // Set up reference frame heirarchies.
	earth->addChild(sun);
	earth->addChild(earthgrid);
	earth->addChild(plane);
	earth->addChild(plane2);

	View *view = new View(earth, earth);
	View *view2 = new View(earth, sun);

	  // Create a manager that will allow access to the scene
	FrameManager* fm = new FrameManager;
	fm->setFrame(earth);

	theWindow->setScene(fm, 0, 0);
	theWindow->getGridPosition(0, 0)->addView(view);
	theWindow->getGridPosition(0, 0)->addView(view2);
	theWindow->getGridPosition(0, 0)->setSkySphereTexture("../Images/StarMap.tif");
	
	sun->setPosition(AU, 0.0, 0.0);

	// Create the actual window, start event handling and animations
	theWindow->startThread();

	  // Compute and update sun's position
	for(double t = 0.0; theWindow->isRunning(); t += M_PI/(1.0e9))
	{
	  sun->setPosition(AU*cos(t), AU*sin(t), 0);
	}

	  // Wait for animations to end
	theWindow->join();

	  // Go home and eat Rocky Road ice cream
	return 0;
}