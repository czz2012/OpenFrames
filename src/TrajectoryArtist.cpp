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

#include <OpenFrames/TrajectoryArtist>
#include <OpenFrames/DoubleSingleUtils>
#include <iostream>
#include <osg/io_utils>

namespace OpenFrames
{

TrajectoryArtist::TrajectoryArtist() 
{
	// Disable lighting computations
	osg::StateSet* stateset = getOrCreateStateSet();
	stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
}

// Not using the copy constructor
TrajectoryArtist::TrajectoryArtist( const TrajectoryArtist &ta, const osg::CopyOp& copyop )
{}

TrajectoryArtist::~TrajectoryArtist()
{
	if(_traj.valid()) _traj->removeArtist(this); 
}

void TrajectoryArtist::setTrajectory(const Trajectory *traj)
{
	if(_traj == traj) return;

	// Unregister from the old trajectory
	if(_traj.valid()) _traj->removeArtist(this);

	// Register with the new trajectory
	_traj = traj;
	if(_traj.valid()) _traj->addArtist(this);
}

osg::BoundingBox TrajectoryArtist::computeBoundingBox() const
{
	_boundingBox.init();
	return _boundingBox;
}

void TrajectoryArtist::RTE_glVertex(osg::Vec3d &point, osg::GLExtensions &glext) const
{
        osg::Vec3f high, low;

        // Split input point into high and low portions
        OpenFrames::DS_Split(point, high, low);

        // Submit to OpenGL
        // Note that vertex attribute must be specified BEFORE glVertex
        glext.glVertexAttrib3fv(1, low._v);
        glVertex3fv(high._v);
}

} //!namespace OpenFrames
