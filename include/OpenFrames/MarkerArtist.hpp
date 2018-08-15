/***********************************
   Copyright 2018 Ravishankar Mathur

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

/** \file MarkerArtist.hpp
 * Declaration of MarkerArtist class.
 */

#ifndef _OF_MARKERARTIST_
#define _OF_MARKERARTIST_

#include <OpenFrames/Export.h>
#include <OpenFrames/TrajectoryArtist.hpp>

namespace OpenFrames
{
  /**
   * \class MarkerArtist
   *
   * \brief This class draws markers at trajectory points.
   *
   * The MarkerArtist is a type of TrajectoryArtist that draws markers at points
   * of a trajectory. Markers can be standard OpenGL points or point sprites
   * that use a specified image. Markers can be drawn at the beginning or end of
   * a Trajectory, or at specified intermediate points.
   */
  class OF_EXPORT MarkerArtist : public TrajectoryArtist
  {
  public:

	/** Indicates which data points should be drawn as markers. */
	enum DrawnMarkers
	{
	  START = 1,        // Draw marker at start point.
	  INTERMEDIATE = 2, // Draw markers at intermediate points, excluding
	                    // the start & end points.
	  END = 4           // Draw marker at end point.
	};

	/** Indicates how intermediate marker spacing is determined. */
	enum IntermediateType
	{
	  TIME = 1, // Draw markers at equally spaced time increments.
	  DISTANCE, // Draw markers at equally spaced distances, with distance
	            // being measured using DataSource as a point.
	  DATA      // Draw markers at equally spaced data points.
	};

	MarkerArtist( const Trajectory *traj = NULL );

	// Copy constructor
	MarkerArtist( const MarkerArtist &ca,
	              const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY );

	/** Standard OSG node methods. */
	virtual Object* cloneType() const { return new MarkerArtist(); }
	virtual Object* clone(const osg::CopyOp& copyop) const { return new MarkerArtist(*this,copyop); }
	virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const MarkerArtist*>(obj)!=NULL; }
	virtual const char* libraryName() const { return "OpenFrames"; }
	virtual const char* className() const { return "MarkerArtist"; }

	/** Set the trajectory to be drawn. */
	virtual void setTrajectory(const Trajectory *traj);

	/** Set the data to be used for plotting x/y/z components */
	bool setXData(const Trajectory::DataSource &src);
	bool setYData(const Trajectory::DataSource &src);
	bool setZData(const Trajectory::DataSource &src);

	/** Set the markers to be used, from enum DrawnMarkers */
	void setMarkers( unsigned int markers );
	void setMarkerColor( unsigned int markers, float r, float g, float b);
	void setMarkerSize(unsigned int size); // Size in pixels
	bool setMarkerImage( const std::string &fname );
	bool setMarkerShader( const std::string &fname );

	/** Specify whether to automatically shrink/grow the marker size as
	    the camera gets closer or farther from the marker. */
	void setAutoAttenuate(bool attenuate);
	bool getAutoAttenuate() const;

	/** Set the intermediate marker parameters */
	void setIntermediateType(IntermediateType type);

	/** Set the intermediate marker spacing */
	void setIntermediateSpacing(double spacing);

	/** Specify if the markers should be computed forward from the start
	    of the trajectory, or backward from the end of the trajectory.
	    Use MarkerType enum values START or END */
	void setIntermediateDirection(DrawnMarkers direction);

	/** Do the actual drawing */
	virtual void drawImplementation(osg::RenderInfo& renderInfo) const;

	/** Tell artist that data was cleared. This is automatically called. */	
	virtual void dataCleared(Trajectory* traj);

	/** Tell artist that data was added. This is automatically called. */	
	virtual void dataAdded(Trajectory* traj);

	/** Compute the auto attenuation coefficients. */
	void computeAttenuation();

  protected:
	virtual ~MarkerArtist();

	/** Inherited from TrajectoryArtist. */
	virtual osg::BoundingBox computeBoundingBox() const;

	/** Verify whether the requested data is valid. */
	void verifyData() const;

        /** Reset shader to default state (circular point) */
        void resetMarkerShader();

        // Data sources for x, y, and z components
	Trajectory::DataSource _dataSource[3];

	unsigned int _markers; // Which points to draw as markers

	IntermediateType _intermediateType; // Type of intermediate markers
	double _intermediateSpacing; // Spacing for intermediate markers
	DrawnMarkers _intermediateDirection; // Intermediate marker direction

	float _startColor[3]; // Starting marker color
	float _endColor[3];   // Ending marker color
	float _intermediateColor[3]; // Intermediate markers color

	mutable bool _dataValid; // If trajectory supports required data
	mutable bool _dataZero; // If we're just drawing at the origin
	mutable bool _shouldAttenuate; // Attenuation needs to be recomputed

        osg::ref_ptr<osg::Shader> _fragShader; // Marker fragment shader
  };

}

#endif
