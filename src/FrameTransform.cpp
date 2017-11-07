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

#include <OpenFrames/FrameTransform.hpp>
#include <osgUtil/CullVisitor>
#include <OpenThreads/ScopedLock>
#include <iostream>
#include <climits>
#include <algorithm>

namespace OpenFrames
{

FrameTransform::FrameTransform()
{
	reset();
}

FrameTransform::~FrameTransform() {}

void FrameTransform::reset()
{
	_disabled = false;
	_followEye = false;
	setPosition(0.0, 0.0, 0.0);
	setAttitude(0.0, 0.0, 0.0, 1.0);
	setScale(1.0, 1.0, 1.0);
	setPivot(0.0, 0.0, 0.0);
}

void FrameTransform::setPosition(const double &x, const double &y, const double &z)
{
	_position[0] = x;
	_position[1] = y;
	_position[2] = z;
	dirtyBound();
}

void FrameTransform::setPosition(const osg::Vec3d &pos)
{
  _position = pos;
  dirtyBound();
}

void FrameTransform::getPosition(double &x, double &y, double &z) const
{
	if(_disabled) x = y = z = 0.0;
	else
	{
	  x = _position[0];
	  y = _position[1];
	  z = _position[2];
	}
}

void FrameTransform::getPosition(osg::Vec3d &pos) const
{
  if (_disabled) pos.set(0.0, 0.0, 0.0);
  else pos = _position;
}

void FrameTransform::setAttitude(const double &rx, const double &ry,
				const double &rz, const double &angle)
{
	_attitude._v[0] = rx;
	_attitude._v[1] = ry;
	_attitude._v[2] = rz;
	_attitude._v[3] = angle;
	dirtyBound();
}

void FrameTransform::setAttitude(const osg::Quat &att)
{
  _attitude = att;
  dirtyBound();
}

void FrameTransform::getAttitude(double &rx, double &ry, double &rz, double &angle) const
{
	if(_disabled)
	{
	  rx = ry = rz = 0.0;
	  angle = 1.0;
	}
	else
	{
	  rx = _attitude._v[0];
	  ry = _attitude._v[1];
	  rz = _attitude._v[2];
	  angle = _attitude._v[3];
	}
}

void FrameTransform::getAttitude(osg::Quat &att) const
{
  if (_disabled) att.set(0.0, 0.0, 0.0, 1.0);
  else att = _attitude;
}

void FrameTransform::setScale(const double &sx, const double &sy, const double &sz)
{
	_scale[0] = sx;
	_scale[1] = sy;
	_scale[2] = sz;
	dirtyBound();
}

void FrameTransform::getScale(double &sx, double &sy, double &sz)
{
	if(_disabled) sx = sy = sz = 1.0;
	else
	{
	  sx = _scale[0];
	  sy = _scale[1];
	  sz = _scale[2];
	}
}

void FrameTransform::setPivot(const double &px, const double &py, const double &pz)
{
	_pivot[0] = px;
	_pivot[1] = py;
	_pivot[2] = pz;
	dirtyBound();
}

void FrameTransform::getPivot(double &px, double &py, double &pz)
{
	if(_disabled) px = py = pz = 1.0;
	else
	{
	  px = _pivot[0];
	  py = _pivot[1];
	  pz = _pivot[2];
	}
}

/** Compute the matrix that will transform a point in the local frame to a
    point in the world frame. Given a transform consisting of a translation,
    rotation, scale, and pivot, the point is first translated wrt the pivot,
    then scaled in the local frame, then rotated to the world frame, then 
    translated in the world frame. 

    Here, "matrix" is a transform from the parent frame to the world frame,
    so we only need to add (premultiply) the local transformations to it.
*/
bool FrameTransform::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
{
	if(_disabled) return false; // Don't do anything

	// Compute the transform relative to the parent node
	if(_referenceFrame == RELATIVE_RF)
	{
	  // If we are following the user's eye (ie for a Sky Sphere), then
	  // first translate for that offset.
	  if(_followEye)
	  {
	    osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
	    if(cv)
	    {
              // Can't use cv->getEyeLocal() since Vec3=Vec3f
	      osg::Vec3d eye = osg::Matrix::inverse(*cv->getModelViewMatrix()).getTrans();
	      matrix.preMultTranslate(eye);
	    }
	  }

	  // "matrix" is the world matrix (from parent to world frame)
	  matrix.preMultTranslate(_position);
	  matrix.preMultRotate(_attitude);
	  matrix.preMultScale(_scale);
	  matrix.preMultTranslate(-_pivot);
	}
	else // ABSOLUTE_RF
	{
	  matrix.makeRotate(_attitude);
	  matrix.postMultTranslate(_position);
	  matrix.preMultScale(_scale);
	  matrix.preMultTranslate(-_pivot);
	}

	return true;
}

/** Compute the matrix that transforms a point in the world frame to a point
    in the local frame. Transforms are applied in the opposite order as in
    the computeLocalToWorldMatrix() function. 

    Here, "matrix" is a transform from the world to the parent frame, so we
    only need to add (postmultiply) local transforms to this.
*/
bool FrameTransform::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
{
	if(_disabled) return false; // Don't do anything

	// Any zero scale leads to a singularity in the matrix
	if(_scale[0] == 0.0 || _scale[1] == 0.0 || _scale[2] == 0.0)
	  return false;

	if(_referenceFrame == RELATIVE_RF)
	{
	  if(_followEye)
	  {
	    osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
	    if(cv)
	    {
	      osg::Vec3d eye = osg::Matrix::inverse(*cv->getModelViewMatrix()).getTrans();
	      //osg::Vec3d eye = cv->getEyeLocal();
	      matrix.postMultTranslate(-eye);
	    }
	  }
	  
	  // "matrix" is the local matrix (from world to parent frame)
	  matrix.postMultTranslate(-_position);
	  matrix.postMultRotate(_attitude.inverse());
	  matrix.postMultScale(osg::Vec3d(1.0/_scale[0], 1.0/_scale[1], 1.0/_scale[2]));
	  matrix.postMultTranslate(_pivot);
	}
	else
	{
	  matrix.makeRotate(_attitude.inverse());
	  matrix.preMultTranslate(-_position);
	  matrix.postMultScale(osg::Vec3d(1.0/_scale[0], 1.0/_scale[1], 1.0/_scale[2]));
	  matrix.postMultTranslate(_pivot);
	}

	return true;
}

TrajectoryFollower::TrajectoryFollower(Trajectory *traj)
  : _follow(NULL), _usingDefaultData(true)
{
	setFollowTrajectory(traj);
  
	_mode = LOOP;
	_data = POSITION + ATTITUDE;

	_offsetTime = 0.0;
	_timeScale = 1.0;
	_paused = false;
	_needsUpdate = false;
	_deltaTime = _pauseTime = 0.0;
	_latestTime = DBL_MAX;
}

TrajectoryFollower::~TrajectoryFollower() {}

void TrajectoryFollower::setFollowTrajectory(Trajectory *traj)
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
  
  // Already following specified trajectory
	if((_trajList.size() == 1) && (_trajList[0] == traj)) return;
  
  // Reset currently followed trajectory pointer
  _follow = NULL;
 
  // Reference new trajectory so that it isn't deleted
  // if it's already being followed
  osg::ref_ptr<Trajectory> tmptraj(traj);
  
  // Clear existing trajectory list and add new trajectory
  _trajList.clear();
  if(traj != NULL) _trajList.push_back(traj);
  
  // Set default data sources if needed
  if(_usingDefaultData) setDefaultData();
  
  // Check for valid data sources
  else _dataValid = _verifyDataSources();
  
  // Indicate that the follower should update its state
	_needsUpdate = true;
}
  
void TrajectoryFollower::followTrajectory(Trajectory *traj)
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);

  if(traj == NULL) return; // Error check
  
  // Already following specified trajectory
  if(std::find(_trajList.begin(), _trajList.end(), traj) != _trajList.end()) return;
  
  // Add new trajectory
  _trajList.push_back(traj);
  
  // Set default data sources if needed
  if(_usingDefaultData) setDefaultData();
  
  // Check for valid data sources
  else _dataValid = _verifyDataSources();
  
  // Indicate that the follower should update its state
  _needsUpdate = true;
}

void TrajectoryFollower::unfollowTrajectory(Trajectory *traj)
{
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);

  // Unfollow all trajectories
  if(traj == NULL)
  {
    _follow = NULL;
    _trajList.clear();
  }
  else // Unfollow specified trajectory
  {
    // Reset currently followed trajectory pointer
    if(_follow == traj) _follow = NULL;
    
    // Remove trajectory if it is followed
    // Note that we use erase-find instead of erase-remove because
    // followed trajectory list is unique
    _trajList.erase(std::find(_trajList.begin(), _trajList.end(), traj));
  }
  
  // Set default data sources if needed
  if(_usingDefaultData) setDefaultData();
  
  // Check for valid data sources
  else _dataValid = _verifyDataSources();
  
  // Indicate that the follower should update its state
  _needsUpdate = true;
}
  
bool TrajectoryFollower::setXData(const Trajectory::DataSource &src)
{
	if(_dataSource[0] == src) return _dataValid; // No changes to be made

	_dataSource[0] = src; // Set new source
  _dataValid = _verifyDataSources(); // Check data validity
	_needsUpdate = true; // Tell follower to update its state
  _usingDefaultData = false;

	return _dataValid;
}

bool TrajectoryFollower::setYData(const Trajectory::DataSource &src)
{
	if(_dataSource[1] == src) return _dataValid;

	_dataSource[1] = src; // Set new source
  _dataValid = _verifyDataSources(); // Check data validity
  _needsUpdate = true; // Tell follower to update its state
  _usingDefaultData = false;

	return _dataValid;
}

bool TrajectoryFollower::setZData(const Trajectory::DataSource &src)
{
	if(_dataSource[2] == src) return _dataValid;

	_dataSource[2] = src; // Set new source
  _dataValid = _verifyDataSources(); // Check data validity
  _needsUpdate = true; // Tell follower to update its state
  _usingDefaultData = false;

	return _dataValid;
}

void TrajectoryFollower::setDefaultData()
{
  // Get DOF for first followed trajectory
  unsigned int dof = _trajList.empty()?0:_trajList[0]->getDOF();
  Trajectory::DataSource dataPoint;
  
  // Use X data if at least 1 DOF
  if(dof >= 1)
  {
    dataPoint._src = Trajectory::POSOPT;
    dataPoint._element = 0;
  }
  else dataPoint._src = Trajectory::ZERO;
  setXData(dataPoint);
  
  // Use X/Y data if at least 2 DOF
  if(dof >= 2)
  {
    dataPoint._src = Trajectory::POSOPT;
    dataPoint._element = 1;
  }
  else dataPoint._src = Trajectory::ZERO;
  setYData(dataPoint);
  
  // Use X/Y/Z data if at least 3 DOF
  if(dof >= 3)
  {
    dataPoint._src = Trajectory::POSOPT;
    dataPoint._element = 2;
  }
  else dataPoint._src = Trajectory::ZERO;
  setZData(dataPoint);
  
  _usingDefaultData = true;
}
  
void TrajectoryFollower::setTimeScale(double timeScale)
{
	if(_timeScale != timeScale)
	{
	  // Compute new time offset to account for change of time scale
	  if(_paused) _deltaTime += _pauseTime*(_timeScale - timeScale);
	  else _deltaTime += _latestTime*(_timeScale - timeScale);

	  _timeScale = timeScale;
	}

	_needsUpdate = true;
}

void TrajectoryFollower::setPaused(bool pause)
{
	if(_paused != pause)
	{
	  _paused = pause;

	  if(_paused) _pauseTime = _latestTime;
	  else _deltaTime += _timeScale*(_pauseTime - _latestTime);
	}

	_needsUpdate = true;
}

void TrajectoryFollower::setOffsetTime(double offsetTime)
{
	_offsetTime = offsetTime;
	_needsUpdate = true;
}

void TrajectoryFollower::reset()
{
	// Reset parameters such that the newly computed time will be the user specified time offset
	_deltaTime = -_timeScale*_latestTime;
	_pauseTime = _latestTime;
	_needsUpdate = true;
}

void TrajectoryFollower::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
	double refTime = nv->getFrameStamp()->getReferenceTime();

  // Make time has changed
	if(_latestTime != refTime)
	{ 
	  if(_latestTime == DBL_MAX) // First call, initialize variables
	  {
	    _latestTime = refTime; // Store the current time
	    reset(); // Initialize times
	  }
	  else _latestTime = refTime; // Just store the current time
    
    // Don't allow followed trajectory list to be modified
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    
    // Follow trajectory as needed
	  if(!_trajList.empty() && (!_paused || _needsUpdate))
	  {
	    // Current simulation time = _offset + _delta + _tscale*_time
	    double time = _offsetTime + _deltaTime;
	    if(_paused) time += _timeScale*_pauseTime;
	    else time += _timeScale*_latestTime;

	    // Prevent trajectories from being modified while reading them
      for(auto traj : _trajList)
      {
        traj->lockData();
      }
      
      // Compute adjusted time based on follow mode
      double tNew = _computeTime(time);
      
      // Choose trajectory based on adjusted time
      _follow = _chooseTrajectory(tNew);

      // Unlock all trajectories except the one being followed
      for(auto traj : _trajList)
      {
        if(traj != _follow) traj->unlockData();
      }

	    // Apply new position/attitude to the FrameTransform
	    FrameTransform *ft = static_cast<FrameTransform*>(node);
      
	    if(_dataValid && (_data & POSITION)) 
	    {
	      _updateState(tNew, POSITION);
	      ft->setPosition(_v1[0], _v1[1], _v1[2]);
	    }

	    if(_data & ATTITUDE) 
	    {
	      _updateState(tNew, ATTITUDE);
	      ft->setAttitude(_a1[0], _a1[1], _a1[2], _a1[3]);
	    }

	    _follow->unlockData(); // Unlock followed trajectory

	    _needsUpdate = false; // Reset update flag
	  }
	}

	// Call nested callbacks and traverse rest of scene graph
	osg::NodeCallback::traverse(node, nv);
}

double TrajectoryFollower::_computeTime(double time)
{
  // Compute start and end times over all trajectories
  double t0 = DBL_MAX;
  double tf = -DBL_MAX;
  for(auto traj : _trajList)
  {
    // Get current trajectory's start & end time
    double traj_t0, traj_tf;
    if(!traj->getTimeRange(traj_t0, traj_tf)) continue;
    if(traj_t0 > traj_tf) std::swap(traj_t0, traj_tf); // Enforce t0<=tf

    // Update global start & end times
    if(traj_t0 < t0) t0 = traj_t0;
    if(traj_tf > tf) tf = traj_tf;
  }

  // Error check
  if((t0 == DBL_MAX) || (tf == -DBL_MAX)) return time;

  // LIMIT mode: don't wrap time
  if(_mode == LIMIT) return time;

  // Otherwise LOOP mode: wrap time to [t0, tf]

  // If [t0, tf] range is too small, then just use t0
  if(tf - t0 <= 8.0*DBL_MIN) return t0;

  // All error checks done, now wrap!
  double tnew = time - std::floor((time - t0)/(tf - t0))*(tf - t0);
  return tnew;
}

Trajectory* TrajectoryFollower::_chooseTrajectory(double time)
{
  // If there is only one trajectory in the list, then use it
  if(_trajList.size() == 1) return _trajList[0];

  // If current trajectory contains given time, then continue using it
  if(_follow && (_follow->getTimeDistance(time) <= 0.0)) return _follow;

  // Find first trajectory that contains given time
  double minTimeDistance = DBL_MAX;
  Trajectory* minTimeDistanceTraj = _trajList[0];
  for(auto traj : _trajList)
  {
    // Get distance from given time to current trajectory time range
    double dist = traj->getTimeDistance(time);

    // Use trajectory if it contains given time
    if(dist <= 0.0) return traj;

    // Otherwise save this as a candidate for "closest" trajectory
    else if(dist < minTimeDistance)
    {
      minTimeDistance = dist;
      minTimeDistanceTraj = traj;
    }
  }

  // No trajectories contain given time, so use the closest trajectory
  return minTimeDistanceTraj;
}

void TrajectoryFollower::_updateState(double time, TrajectoryFollower::FollowData data)
{
  int val, index = 0;

  // Get list of times for the followed Trajectory
  const Trajectory::DataArray& times = _follow->getTimeList();

  // Number of position points supported by Trajectory
  unsigned int numPoints;
  if(data == POSITION)
    numPoints = _follow->getNumPoints(_dataSource);
  else
    numPoints = _follow->getNumAtt();
  
  // If no points or ZERO used for position, then directly zero out state
  if(numPoints == 0 || numPoints == UINT_MAX)
  {
    if(data == POSITION)
      _v1.set(0.0, 0.0, 0.0);
    else
      _a1.set(0.0, 0.0, 0.0, 1.0);
    return;
  }

  // Find requested time in the Trajectory
  val = _follow->getTimeIndex(time, index);

  if(val >= 0) // Time not out of range, so interpolate
  {
    // Check that time index is less than actual number of points
    if(index < (int)numPoints)
    {
      // Get the first point used for interpolation
      if(data == POSITION)
        _follow->getPoint(index, _dataSource, _v1._v);
      else
        _follow->getAttitude(index, _a1[0], _a1[1], _a1[2], _a1[3]);
      
      // Interpolate if the two times are not equal
      if((index+1 < (int)numPoints) && (times[index] != times[index+1]))
      {
        // Get second interpolation point and do interpolation
        if(data == POSITION)
        {
          _follow->getPoint(index+1, _dataSource, _v2._v);
          double frac = (time - times[index])/(times[index+1] - times[index]);
          _v1 = _v1 + (_v2-_v1)*frac; // Linear interpolation
        }
        else
        {
          _follow->getAttitude(index+1, _a2[0], _a2[1], _a2[2], _a2[3]);
          double frac = (time - times[index])/(times[index+1] - times[index]);
          _a1.slerp(frac, _a1, _a2); // Spherical interpolation for attitude
        }
      }
    }
    else // Otherwise use last available point
    {
      if(data == POSITION)
        _follow->getPoint(numPoints-1, _dataSource, _v1._v);
      else
        _follow->getAttitude(numPoints-1, _a1[0], _a1[1], _a1[2], _a1[3]);
    }
  }
  else if(val == -1) // Time out of range
  {
    if(index < 0) // Requested time before first time
    {
      if(data == POSITION)
        _follow->getPoint(0, _dataSource, _v1._v);
      else
        _follow->getAttitude(0, _a1[0], _a1[1], _a1[2], _a1[3]);
    }
    else // Requested time after last time
    {
      if(data == POSITION)
        _follow->getPoint(numPoints-1, _dataSource, _v1._v);
      else
        _follow->getAttitude(numPoints-1, _a1[0], _a1[1], _a1[2], _a1[3]);
    }
  }
  else if(val == -2) // Error in search (endless iterations)
  {
    std::cerr<< "FrameTransform::_updateState() error: Requested time not found in a reasonable number of iterations!" << std::endl;
  }
  else // Unhandled return value from getTimeIndex()
  {
    std::cerr<< "FrameTransform::_updateState() error: Unhandled return value!" << std::endl;
  }
}

TimeManagementVisitor::TimeManagementVisitor()
{
  setTraversalMode(TRAVERSE_ALL_CHILDREN);

	_pauseState = false;
	_changePauseState = _changeOffsetTime = _changeTimeScale = false;
	_offsetTime = 0.0;
	_timeScale = 1.0;
	_reset = false;
}

TimeManagementVisitor::~TimeManagementVisitor() {}

void TimeManagementVisitor::setPauseState(bool changePauseState, bool pauseState) 
{ 
	_changePauseState = changePauseState;
	_pauseState = pauseState; 
}

void TimeManagementVisitor::setOffsetTime(bool changeOffsetTime, double offsetTime)
{
	_changeOffsetTime = changeOffsetTime;
	_offsetTime = offsetTime;
}

void TimeManagementVisitor::setTimeScale(bool changeTimeScale, double timeScale)
{
	_changeTimeScale = changeTimeScale;
	_timeScale = timeScale;
}

void TimeManagementVisitor::apply(osg::Transform &node)
{
	// Make sure current node is a FrameTransform
	FrameTransform *ft = dynamic_cast<FrameTransform*>(&node);
	if(ft)
	{
	  // Make sure FrameTransform has a TrajectoryFollower callback
	  TrajectoryFollower *tf = dynamic_cast<TrajectoryFollower*>(ft->getUpdateCallback());
	  if(tf) 
	  {
	    if(_changePauseState) tf->setPaused(_pauseState);
	    if(_changeOffsetTime) tf->setOffsetTime(_offsetTime);
	    if(_changeTimeScale) tf->setTimeScale(_timeScale);
	    if(_reset) tf->reset();
	  }
	}

	// Traverse & pause children if needed
	osg::NodeVisitor::traverse(node);
}
  
} // !namespace OpenFrames
