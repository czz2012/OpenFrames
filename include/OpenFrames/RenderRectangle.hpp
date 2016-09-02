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

#ifndef _OF_RENDERRECTANGLE_
#define _OF_RENDERRECTANGLE_

#include <OpenFrames/Export.h>
#include <OpenFrames/DepthPartitionNode.hpp>
#include <OpenFrames/FrameManager.hpp>
#include <OpenFrames/Sphere.hpp>
#include <OpenFrames/View.hpp>
#include <osg/Camera>
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <osgViewer/View>
#include <vector>

namespace OpenFrames
{

/**********************************************************
 * Ravi Mathur
 * OpenFrames API, class RenderRectangle
 * This class encapsulates a rectangle in which a scene can be
 * rendered.  It provides decorations for the scene, such as a
 * border around the rectangle.  It also automatically analyzes
 * a scene and (if needed) renders it in multiple stages, in
 * case there are large z distances involved.
**********************************************************/
class OF_EXPORT RenderRectangle : public osg::Referenced
{
  public:
	typedef std::vector<osg::ref_ptr<View> > ViewList;

	RenderRectangle();

	/** Set the FrameManager containing the scene to be viewed */
	void setFrameManager(FrameManager *fm);
	inline FrameManager* getFrameManager() const { return _frameManager.get(); }

	osgViewer::View* getSceneView() const { return _sceneView.get(); }

	/* Set the size of the viewport to render into */
	void setViewport(int x, int y, int w, int h);

	/** Set the border color to red if selected, and green if deselected. */
	void setSelected(bool select);
	bool getSelected();

	/** Set whether the "selected" border rectangle is shown or not */
	void setShowBorder(bool show);

	/** Set the sky sphere texture */
	void setSkySphereTexture(const std::string& fname);

        /** Set the background color if no texture is specified */
        void setBackgroundColor(float r, float g, float b);

	/** Enable/disable the automatic depth partitioner */
	void setDepthPartitioningEnabled(bool enable)
	{ _depthPartition->setActive(enable); }
	
	/** Add/remove a view to the view list that can be iterated through */
	void addView(View *view);    // Adds view to the end of the view list
	void removeView(View *view); // Removes all instances of view
	void removeAllViews();       // Clears entire view list

	/** Iterate through the view list */
	void nextView();
	void previousView();

	/** Select a particular view if it exists in the view list. */
	void selectView(View *view);
	void selectView(unsigned int newView);

	/** Get the current View, or the default View if none have been set. */
	View* getCurrentView();

	/** Apply the current View's perspective to the SceneView. */
	void applyCurrentPerspective();

  protected:
	virtual ~RenderRectangle();
	void _init();

	void selectCurrentView(); // Just make sure current view is selected

	ViewList _views; // All of the added Views
	osg::ref_ptr<View> _defaultView; // Used if no Views have been added
	unsigned int _currView; // Currently active View

	// Contains the ReferenceFrame scene, and any decorations such as
	// a box around this RenderRectangle
	osg::ref_ptr<osgViewer::View> _sceneView;

	// The root of the entire scene. This node makes sure that the scene is
	// rendered correctly, even if it has large depth ranges.
	osg::ref_ptr<DepthPartitionNode> _depthPartition;

	osg::ref_ptr<osg::Group> _scene; // Everything to be drawn
	osg::ref_ptr<osg::Camera> _border; // Border around rectangle
	osg::ref_ptr<Sphere> _skySphere;

	// Manager for access to the ReferenceFrame scene
	osg::ref_ptr<FrameManager> _frameManager;
};

}

#endif
