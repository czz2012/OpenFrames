/***********************************
 Copyright 2017 Ravishankar Mathur
 
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

#include <OpenFrames/OpenVRDevice.hpp>
#include <OpenFrames/ReferenceFrame.hpp>
#include <osg/Matrixd>
#include <osg/Notify>
#include <osg/MatrixTransform>

// Assume 3 stub tracked devices here: HMD + 2 base stations
const unsigned int numTrackedDevices = 3;

namespace OpenFrames{

  /*************************************************************/
  OpenVREvent::VREvent::VREvent()
  : _ovrEvent(NULL), _controllerState(NULL)
  {}
  
  /*************************************************************/
  OpenVREvent::VREvent::~VREvent()
  {}
  
  /*************************************************************/
  bool OpenVRDevice::initVR()
  {
    osg::notify(osg::NOTICE) << "Using OpenVR stub" << std::endl;
    
    // Set texture size similar to what OpenVR would return
    _width = 1512;  // 1.4*1080
    _height = 1680; // 1.4*1200
    osg::notify(osg::NOTICE) << "VR eye texture width = " << _width << ", height = " << _height << std::endl;

    // Update the per-eye projection matrices
    // The view offset matrices will be computed per-frame since IPD can change
    updateProjectionMatrices();
    
    // Allocate render data for each possible tracked device
    // The render data struct is a light wrapper, so there is no size concern here
    _deviceIDToModel.resize(numTrackedDevices);

    // Get render models for controllers and other devices
    updateDeviceRenderModels();
    
    return (_isInitialized = true);
  }
  
  /*************************************************************/
  void OpenVRDevice::shutdownVR()
  {
    _deviceNameToGeode.clear();
    _deviceIDToModel.clear();
    _deviceModels->removeChildren(0, _deviceModels->getNumChildren());
    _isInitialized = false;
  }
  
  /*************************************************************/
  void OpenVRDevice::updateDeviceRenderModels()
  {
    // Loop through all possible tracked devices except the HMD (already loaded)
    for(unsigned int deviceID = 1; deviceID < numTrackedDevices; ++deviceID)
    {
      // Set up device render model
      setupRenderModelForTrackedDevice(deviceID);
    }
  }
  
  /*************************************************************/
  void OpenVRDevice::setupRenderModelForTrackedDevice(uint32_t deviceID)
  {
    // Get name of tracked device
    std::string deviceName;
    if(deviceID == 0) deviceName = "HMD_Stub";
    else deviceName = "BaseStation_Stub";
    
    // Find device data by name
    DeviceGeodeMap::iterator i = _deviceNameToGeode.find(deviceName);
    
    // If not found, then load device data
    if(i == _deviceNameToGeode.end())
    {
      osg::notify(osg::NOTICE) << "OpenFrames::OpenVRDeviceStub: Setting up render data for device " << deviceName << std::endl;

      float radius = 0.1f;
      float height = 0.2f;
      
      // Create device model's render model and add it to the render group
      osg::Geode *geode = new osg::Geode;
      geode->addDrawable(new osg::ShapeDrawable(new osg::Capsule(osg::Vec3(),radius, height)));
      _deviceNameToGeode[deviceName] = geode;
    }
    
    // Set up device model if needed
    if(_deviceIDToModel[deviceID]._modelTransform == NULL)
    {
      osg::notify(osg::NOTICE) << "OpenFrames::OpenVRDeviceStub: Setting up render model for device " << deviceName << deviceID << std::endl;
      
      // Create device model's transform and add it to the group of all devices
      osg::MatrixTransform *xform = new osg::MatrixTransform;
      xform->addChild(_deviceNameToGeode[deviceName]);
      _deviceIDToModel[deviceID]._modelTransform = xform;
      _deviceIDToModel[deviceID]._class = NONE;
      _deviceModels->addChild(xform);
    }
  }
  
  /*************************************************************/
  void OpenVRDevice::updateProjectionMatrices()
  {
    // Create right/left/center projection matrices. Using unit depth minimizes
    // precision losses in the projection matrix
    _rightEyeProj.makePerspective(110.0, (double)_width/(double)_height, 1.0, 2.0);
    _leftEyeProj.makePerspective(110.0, (double)_width/(double)_height, 1.0, 2.0);

    // Center projection is average of right and left
    // OSG doesn't have a matrix addition for Matrixd (facepalm)
    osg::Matrixf rightEyeProjf = _rightEyeProj;
    osg::Matrixf leftEyeProjf = _leftEyeProj;
    _centerProj = (rightEyeProjf + leftEyeProjf)*0.5;
  }

  /*************************************************************/
  void OpenVRDevice::updateViewOffsets()
  {
    // Simulate raw left/right eye vectors relative to HMD origin
    osg::Vec3d rightEyeRaw(0.03, 0.01, -0.01);
    osg::Vec3d leftEyeRaw(-0.03, 0.01, -0.01);
    
    // Compute view offsets from raw offset vectors
    computeViewOffsets(rightEyeRaw, leftEyeRaw);
  }

  /*************************************************************/
  void OpenVRDevice::waitGetPoses()
  {
    osg::Matrixf matDeviceToWorld; // Device to World transform

    // Simulate HMD pose in meters
    matDeviceToWorld.makeIdentity(); // Look at scene head-on from the OpenVR origin
    matDeviceToWorld(3, 1) = 1.6764 - _userHeight; // 5'6" simulated user height in meters
    _deviceIDToModel[0]._rawDeviceToWorld = matDeviceToWorld;

    // Apply apply translational offset and convert from meters to world units
    matDeviceToWorld(3, 0) = matDeviceToWorld(3, 0)*_worldUnitsPerMeter;
    matDeviceToWorld(3, 1) = matDeviceToWorld(3, 1)*_worldUnitsPerMeter;
    matDeviceToWorld(3, 2) = matDeviceToWorld(3, 2)*_worldUnitsPerMeter;

    // Invert since we want World to HMD transform
    _hmdPose.invert(matDeviceToWorld);
    
    // Simulate pose for base stations
    for(unsigned int i = 1; i < numTrackedDevices; ++i)
    {
      _deviceIDToModel[i]._valid = true; // Always valid
      
      // Set simulated base station pose in meters
      if(i == 1)
      {
        matDeviceToWorld.makeRotate(osg::Quat(10.0, osg::Vec3d(1, 0, 0)));
        //matDeviceToWorld.postMultTranslate(osg::Vec3d(-3, 3, -3));
        matDeviceToWorld.postMultTranslate(osg::Vec3d(0, 2, -1));
      }
      else if(i == 2)
      {
        matDeviceToWorld.makeRotate(osg::Quat(10.0, osg::Vec3d(0, 1, 0)));
        //matDeviceToWorld.postMultTranslate(osg::Vec3d(2, 3, -2));
        matDeviceToWorld.postMultTranslate(osg::Vec3d(0.1, 2, -0.3));
      }
      
      matDeviceToWorld(3, 1) -= _userHeight; // Subtract user's height, OpenVR world is Y-up
      _deviceIDToModel[i]._rawDeviceToWorld = matDeviceToWorld;

      // Apply translational offset and convert from meters to world units
      matDeviceToWorld(3, 0) = matDeviceToWorld(3, 0)*_worldUnitsPerMeter;
      matDeviceToWorld(3, 1) = matDeviceToWorld(3, 1)*_worldUnitsPerMeter;
      matDeviceToWorld(3, 2) = matDeviceToWorld(3, 2)*_worldUnitsPerMeter;

      // Since the device model is assumed in meters, we need to scale it to world units
      // The normals will need to be rescaled, which is done by the containing Camera
      matDeviceToWorld.preMultScale(osg::Vec3d(_worldUnitsPerMeter, _worldUnitsPerMeter, _worldUnitsPerMeter));
      
      // Set base station model's location from its pose
      _deviceIDToModel[i]._modelTransform->setMatrix(matDeviceToWorld);
    }
  }
  
  /*************************************************************/
  void OpenVRDevice::submitFrame(GLuint rightEyeTexName, GLuint leftEyeTexName)
  {
    // Nothing to do here
  }
  
  /*************************************************************/
  bool OpenVRDevice::pollNextEvent(OpenVREvent *event)
  {
    // Nothing to do here
    return false;
  }
  
  /*************************************************************/
  bool OpenVREventDevice::checkEvents()
  {
    // Nothing to do here
    return false;
  }
  
  /*************************************************************/
  bool OpenVRTrackball::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
  {
    // Just call parent trackball handler
    return FollowingTrackball::handle(ea, us);
  }
  
} // !namespace OpenFrames
