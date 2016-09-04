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

#ifndef _OF_TRAJECTORYARTIST_
#define _OF_TRAJECTORYARTIST_

#include <OpenFrames/Export.h>
#include <OpenFrames/Trajectory.hpp>
#include <osg/Drawable>
#include <osg/ref_ptr>
#include <vector>

namespace OpenFrames
{

/***********************************************************
 * Ravi Mathur
 * OpenFrames API, class TrajectoryArtist
 * This abstract class provides the framework that is used to interpret
 * and draw a given Trajectory object.  What is actually drawn is left
 * to deriving classes; this class just stores the Trajectory and line
 * styles that should be used for drawing.
***********************************************************/
class OF_EXPORT TrajectoryArtist : public osg::Drawable
{
  public:
	TrajectoryArtist();

	// Copy constructor
	TrajectoryArtist( const TrajectoryArtist &ta,
	               const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY );

	/** Standard OSG Node methods */
	virtual bool isSameKindAs(const Object* obj) const { return dynamic_cast<const TrajectoryArtist*>(obj) != NULL; }
	virtual const char* libraryName() const { return "OpenFrames"; }
	virtual const char* className() const { return "TrajectoryArtist"; }

	/** Set the trajectory to be drawn. */
	virtual void setTrajectory(const Trajectory *traj);
	inline const Trajectory* getTrajectory() const {return _traj.get();}

	/** Called by the trajectory when its data is cleared. Must be
	    implemented by derived classes. */
	virtual void dataCleared() = 0;

	/** Called by the trajectory when data is added to it. Must be
	    implemented by derived classes. */
	virtual void dataAdded() = 0;

  protected:
	virtual ~TrajectoryArtist();

	/** Inherited from osg::Drawable */
	virtual osg::BoundingBox computeBoundingBox() const;

        /** Convert double vector to two float vectors and submit
            to OpenGL using rendering relative to the eye. */
        void RTE_glVertex(osg::Vec3d &point, osg::GLExtensions &glext) const;

	osg::ref_ptr<const Trajectory> _traj; // Trajectory to be drawn

        osg::ref_ptr<osg::Program> _program; // GLSL program
};

}

#endif
