package
{
	import flash.display.*;
	import flash.events.KeyboardEvent;
	import flash.events.MouseEvent;
	import flash.events.TimerEvent;
	import flash.geom.Matrix;
	import flash.text.TextField;
	import flash.text.TextFormat;
	import flash.utils.Dictionary;
	import flash.utils.Timer;
	
	import mx.controls.Alert;
	import mx.controls.HSlider;
	import mx.controls.Image;
	import mx.core.Application;
	import mx.core.UIComponent;
	import mx.managers.ToolTipManager;
	import flash.events.HTTPStatusEvent;
	//import mx.controls.Label;	
	/**
	 * This class consists of the logic for drawing the space. One svimage 
	 * object is created for each space 
	 */	
		
	public class SVImage extends Image
	{
		//constants used for array index 
		private const SPACE_X1:int 		 = 0;
		private const SPACE_X2:int 		 = 3;
		private const SPACE_Y1:int 		 = 2;
		private const SPACE_Y2:int 		 = 5;
		private const ENTITY_X:int 		 = 0;
		private const ENTITY_Y:int 		 = 1;
		private const ENTITY_TYPE:int	 = 2;
		private const ENTITY_ID:int 	 = 3;
		private const MAX_SERVER_CPU:Number = 0.75;
		
		//constants for dragging 
		private const DRAGGING:int		 = 1;
		private const SELECTING:int 	 = 2;
		private const ZOOM_SELECTING:int = 3;
		
		private var transImage:Transformation; 
		private var timerDraw:Timer;
		private var spaceBounds:Array;
		private var realEntityData:Array;
		private var ghostEntityData:Array;
		private var cellBoundary:Array;
		private var chunkBounds:Array;
		private var entitySize:int = 4;
		//private var entitySize:int = 6;		
		private var replayer:ReplayerData;
		private var cellRect:Array;
		//private var cellRect:MFRectangle;
		private var cellRects:Dictionary;
		private var cellDataIds:Array;
		private var cellDataIps:Array;
		private var cellDataLoads:Array;
		private var typeNames:Object;		
		private var imageWidth:int	= 592;
		private var imageHeigth:int	= 532;		
		private var entityArray:Array;		
		
		//variables used in dragging 
		private var lastMoveXPos:int	 	= 0;
		private var lastMoveYPos:int		 = 0;       
		private var leftToTravel:int;
		private var mode:int 		= 1;
		private var movementSave:Array		  = new Array(0,0);
		private var xTickOffsetCounter:Number = 0;
		private	var yTickOffsetCounter:Number = 0; 
		
		//this image is used for overlay
		private var imageForOverlay:Image; 		
		
		//Flag used to check whether Display Entities checkbox is selected or not
		private var CanDisplayEntities:Boolean = true;
		
		private var isSingleCell:Boolean = false;
		
		//stores references of currently displayed tooltips
		private var tooltipArray:Array;		
		private var iterator:int = 0;		
		
		//declaration of variables for configuration 
		private var config:ConfigurationData;		
		
		//Zoom slider related variables 
		private var slider:HSlider;
		private var SliderMaxValue:Number;
		private var SliderMinValue:Number;
		private var flag:uint  = 0;
				
		private var space:Shape = new Shape();
		private var space1:Shape=new Shape();
		private var currentSpace:String;
		//private var lab:Label=new Label();
		
		//Declaration of variables for Rubberbanding.
		private var rubberBand:RubberBander = new RubberBander();
			
		//values initialised for working slider logic 
		private var xPos:int;
		private var yPos:int;
		private var mouseDistance:int = 0;
		
		//images for drawing the ticks
		/*private var arrowDown:Image = new Image();
		private var arrowRight:Image = new Image();*/
		
		private var xdown:int = 0;
		private var ydown:int = 0;
		
		private var cmt:CellAppMgrTalker;
		
		private var iconpaths:Dictionary;
		private var icondata:Boolean		= false;
		private  var bitmapData:BitmapData	= new BitmapData(imageWidth,imageHeigth);
        
      
		//Constructor of svimage Class
		public function SVImage(user:String, space:String, width:int, height:int,configTemp:ConfigurationData,sliderTemp:HSlider)
		{
			//Alert.show(Application.application.catalogue1.lastResult.catalogue.image[0].path.toString());
			//Application.application.addChild(lab);
			currentSpace=/*"Space "+*/space;
			transImage	    = new Transformation();
			imageForOverlay	= new Image();
			imageForOverlay.maintainAspectRatio=false;
			imageForOverlay.maxHeight=200;
			imageForOverlay.maxWidth =200;
			imageForOverlay.visible = false;
			
			//imageForOverlay.errorString="Image you have selected was deleted";
			
			//for drawing image on bitmapdata it should be a UI component so add it to application
			Application.application.addChild(imageForOverlay);
			
			//these two arrays are used to display tooltip on mouse over on entity
			entityArray	   = new Array();
			tooltipArray   = new Array();
			slider		   = sliderTemp;
			SliderMaxValue = slider.maximum;
			SliderMinValue = slider.minimum;
			config		   = configTemp;
			SetSize(width, height);
			imageWidth     		= width;
			imageHeigth    		= height;
			transImage.width 	= width;
			transImage.height 	= height;
					
			//replayer=new ReplayerData(UID,spaceId);
			replayer = new ReplayerData(user,space,transImage);
			
			if(replayer.spaceBounds!=null)
			{
				spaceBounds= replayer.spaceBounds ;
				transImage.ZoomToSpaceBounds(spaceBounds);
			}
			this.addEventListener(KeyboardEvent.KEY_DOWN,onKeyDown);
			this.addEventListener(MouseEvent.MOUSE_DOWN,onLMouseDown);
			this.addEventListener(MouseEvent.MOUSE_MOVE,onMouseMove);
			this.addEventListener(MouseEvent.MOUSE_UP,onLMouseUp);
			this.addEventListener(MouseEvent.MOUSE_WHEEL,onMouseWheel);			
			timerDraw = new Timer(100);
			iconpaths = new Dictionary();
			/*config.iconarray depends on httpservice result so it is hard to except
			when it will get so try for each second until we get result once got 
			*/			
			if(config.iconarray && config.EntitySize)
			{
				icondata = true;
				for(var i:int=0;i<config.iconarray.length;i++)
				{
					if(config.iconarray[i].icon_path!="None")
					{
						
						var temp:Image=new Image();
						temp.maintainAspectRatio=false;
						temp.maxHeight=50;
						temp.maxWidth=50;
																		
						temp.source=config.iconarray[i].icon_path;
						//temp.height=temp.height*config.EntitySize;
						//temp.width=temp.width*config.EntitySize;
						temp.visible=false;
						
						Application.application.addChild(temp);
						iconpaths[config.iconarray[i].entity_type]=temp;
						//Alert.show(iconpaths[config.iconarray[i].entity_type].height.toString());
					}
				}
			}
			timerDraw.addEventListener(TimerEvent.TIMER,OnTimer);
			timerDraw.start();
		}
		
		//Empty keydown handler to handle the key down in case of RubberBanding. 
		public function onKeyDown(event:KeyboardEvent):void
		{
		}
		
		/**
		 * Left mouse button down handler for RubberBanding or Dragging depending upon
		 *  the status of Shift key
		 * */
		public function onLMouseDown(event:MouseEvent):void
		{
			if(event.shiftKey) //if Rubberbanding
			{
				timerDraw.stop();
				replayer.stopTimer();
				rubberBand.onMouseEvent(event);
				setMode(ZOOM_SELECTING);
			}
			else
			{
				movementSave[0]=event.localX;
				movementSave[1]=event.localY;
				setMode( DRAGGING );
				xdown=event.localX;
				ydown=event.localY;
				//timerDraw.stop();				
			}
		}
		
		/**
		 * 
		 * Mouse wheel handler to Zoom In or Zoom Out the space.
		 */
		public function onMouseWheel(event:MouseEvent):void
		{
			var x:int=event.localX;
			var y:int=event.localY;
			if(event.delta>0 && slider.value<SliderMaxValue)
			{
				if(slider.value<SliderMaxValue && transImage.XPageSize<=transImage.OrigXPageSize)
					slider.value+=(SliderMaxValue-SliderMinValue)/8.0;
				Zoom(x, y, 2);
			}
			
			if(event.delta<0 && slider.value<=SliderMaxValue)
			{
				if(slider.value>SliderMinValue && transImage.XPageSize>=transImage.OrigXPageSize/SliderMaxValue)
					slider.value-=(SliderMaxValue-SliderMinValue)/8.0;
				Zoom(x, y, 0.5);
			}
		}

		/**
		 * Mouse Move handler to Drag or Rubberbanding or show tooltip
		 */
		public function onMouseMove(event:MouseEvent):void
		{
			lastMoveXPos = event.localX;
			lastMoveYPos = event.localY;
			Application.application.cursorlbl.x=Application.application.svtile.x+this.width-250;
			Application.application.cursorlbl.y=Application.application.svtile.y+this.height+25;
			Application.application.cursorlbl.text="Cursor Position:("+transImage.iTransformX(event.localX).toFixed(1)+","+
			transImage.iTransformY(event.localY).toFixed(1)+") metres";						
			// handle drag
			if (mode == DRAGGING && event.buttonDown)//m_leftDown:
			{
				mouseDistance++;
				
				// calculate new window - don't let user scroll outside +-13km boundary.
				var gx:int = event.localX;
				var gy:int = event.localY;
				var dx:int = gx - movementSave[0];
				var dy:int = gy - movementSave[1];
				leftToTravel -= (Math.abs(dx) + Math.abs(dy));
				if (leftToTravel > 0)
					return;
				leftToTravel = 0;
				movementSave[0] =  gx;
				movementSave[1]= gy;
				var d2:Array = transImage.iTransformDist( dx, dy );
				xPos = transImage.XPosition - d2[0];
				yPos = transImage.YPosition - d2[1];
				var returnArray:Array=transImage.makeNearestValidWindow(xPos, yPos, transImage.getXPageSize(), transImage.getYPageSize());
				
				// update tick mark counters
				var temp:Array=transImage.transformDist( transImage.XPosition - returnArray[0], transImage.YPosition - returnArray[1] );
				xTickOffsetCounter += temp[0];
				yTickOffsetCounter += temp[1];
				
				// finally update drawing.
				transImage.XPosition = returnArray[0];//newXPos
				transImage.YPosition = returnArray[1];//newYPos
				transImage.YPageSize = returnArray[2];//newXPageSize
				
				transImage.IsZoomed=true;
				if(mouseDistance>=2)
				{
					
					Refresh();
					
					mouseDistance=0;
				}
				d2          = null;
				temp        = null;
				returnArray = null;
			}
			else if(mode == ZOOM_SELECTING)   //Rubberbanding
			{
				rubberBand.onMouseEvent( event );
				if(rubberBand.ShiftLMouseDown)
					Refresh();
			}
			else      //onMouseOver but not Drag and Rubberbanding
			{
				//delete previous tooltips if any
				for (iterator=0;iterator<tooltipArray.length;iterator++)
				{
					ToolTipManager.destroyToolTip(tooltipArray.pop());
				}
				
				//create tooltip for entity data
				for(iterator=0;iterator<entityArray.length;iterator++)
				{
					if(event.localX<=entityArray[iterator][0]+3 &&event.localX>=entityArray[iterator][0]-3 &&event.localY<=entityArray[iterator][1]+3 &&event.localY>=entityArray[iterator][1]-3 && CanDisplayEntities)
					{
						tooltipArray.push(ToolTipManager.createToolTip(typeNames[entityArray[iterator][2]]+"#"+entityArray[iterator][3].toString()+"["+int(entityArray[iterator][4]).toString()+","+int(entityArray[iterator][5]).toString()+"]",event.localX+16,event.localY+56));
						break;
					}
				}
			}
		}
		
		/**
		 * Left mouse button Up handler for Completing the Rubberbanding and 
		 * setting the mode to dragging
		 */
		public function onLMouseUp(event:MouseEvent):void
		{
			
			if(mode == ZOOM_SELECTING)
			{
				replayer.timerHandler();
				
				rubberBand.onMouseEvent( event );
				
				var rb:RubberBander = rubberBand;
				var x1:int = transImage.iTransformX( Math.min(rb.m_stpoint_x,rb.m_endpoint_x) );
				var y1:int = transImage.iTransformY( Math.min(rb.m_stpoint_y,rb.m_endpoint_y) );
				var x2:int = transImage.iTransformX( Math.max(rb.m_stpoint_x,rb.m_endpoint_x) );
				var y2:int = transImage.iTransformY( Math.max(rb.m_stpoint_y,rb.m_endpoint_y) );
				transImage.ZoomToRect( x1, y1, x2, y2 );
				transImage.IsZoomed = true;
				//Refresh();
				timerDraw.start();
				replayer.startTimer();
				
				rb = null;	//release the memory
			}
			else
			{
				if(xdown==event.localX && ydown==event.localY)
				{
					replayer.changeCell(transImage.iTransformX(event.localX),transImage.iTransformY(event.localY));
				}
				replayer.timerHandler();
				Refresh();
				//timerDraw.start();
				
			}
			setMode( DRAGGING );
		}
		
		/**
		 * Function used to set the mode
		 */
		public function setMode( Mode:int ):void
		{
			this.mode = Mode;
			if( Mode == DRAGGING)
			{
				// How sensitive it is to decide between select and drag.
				leftToTravel = 3;
			}
			Refresh();
		}
		
		/**
		 * Function for getting the flag for checking that "Display Entities"
		 *  checkbox is selected or not 
		 */
		public function get DisplayEntities():Boolean
		{
			return CanDisplayEntities;
		}
		
		/**
		 * Function for setting the flag for checking that "Display Entities" 
		 * checkbox is selected or not 
		 */
		public function set DisplayEntities(bFlag:Boolean):void
		{
			CanDisplayEntities = bFlag;
			Refresh();
		}
		
		/**
		 * Function to set the size of Space
		 */
		public function SetSize(width:int, height:int):void
		{
			transImage.SetSize(width, height - 60);
			if(width>50)
				imageWidth = width - 50;
			else
				imageWidth = width;
			if(height>60)
				imageHeigth = height - 60;
			else
				imageHeigth=height;
		}
		
		/**
		 * Timer event handler to redraw after some time 
		 */
		private function OnTimer(event:TimerEvent=null):void
		{
			draw();
		}
		
		/**
		 * Function to redraw the space
		 */
		private function Refresh():void
		{
			draw(true);
		}
		
		/**
		 * Function to draw the space
		 */
		private function draw(redraw:Boolean = false):void
		{
			//all the drawing is done on shape object because it has graphics class	
			space.graphics.clear();
			
			//var space1:Shape=new Shape();
			space1.graphics.clear();
			bitmapData.dispose();
			try
			{
				bitmapData=new BitmapData(imageWidth,imageHeigth);
			}catch(error:Error)
			{
				Alert.show("error window size is too small");
				return;
			}
			//draw only if replayer has data
			if(replayer.spaceBounds!=null)
			{
				/*config.iconarray depends on httpservice result so it is hard to except
				when it will get so try for each second untill we get result once got 
				*/
				if(!icondata)
				{
					if(config.iconarray && config.EntitySize)
					{
						icondata=true;
						for(var i:int=0;i<config.iconarray.length;i++)
						{
							if(config.iconarray[i].icon_path!="None")
							{
								
								var temp:Image=new Image();
								temp.maintainAspectRatio=false;
								temp.maxHeight=50;
								temp.maxWidth=50;
								temp.source=config.iconarray[i].icon_path;
								
								//temp.height=temp.height*config.EntitySize;
								//temp.width=temp.width*config.EntitySize;
								temp.visible=false;
								Application.application.addChild(temp);
								iconpaths[config.iconarray[i].entity_type]=temp;
								//Alert.show(	iconpaths[config.iconarray[i].entity_type].height.toString());							
							}
						}
					}
				}
				spaceBounds=replayer.spaceBounds;
				if(redraw == false && transImage.IsZoomed == false)
				{
					transImage.ZoomToSpaceBounds(spaceBounds);
					slider.value=SliderMinValue;
				}
					
				realEntityData=replayer.entityData;
				ghostEntityData=replayer.ghostEntityData;
				//cellBoundary=replayer.cellBoundary;				
				cellRect=replayer.cellRect;
				//cellRects=replayer.cellRects;
				cellDataIps=replayer.cellDataIps;
				cellDataIds=replayer.cellDataIds;
				cellDataLoads=replayer.cellDataLoads;
				chunkBounds=replayer.chunkBounds;
				typeNames=replayer.typeNames;
				//if(cellRects==null)
				//{
				var temp123:MFRectangle=new MFRectangle(transImage.iTransformX(-3),transImage.iTransformY(transImage.height+3),
										transImage.iTransformX(transImage.width+3),transImage.iTransformY(-3));
				cellRects=new Dictionary();
				cellBoundary=new Array();				
				replayer.cmt.visit(temp123,cellBoundary,cellRects);	
				//Alert.show(cellRects.length.toString());
				//}			
			}		
			if(replayer.spaceBounds!=null)
			{
				if(Application.application.cursorlbl.text=="")
				{
					Application.application.cursorlbl.text="Cursor Position:";
				}
				Application.application.cursorlbl.x=Application.application.svtile.x+this.width-250;
				Application.application.cursorlbl.y=Application.application.svtile.y+this.height+25;				
				drawSpaceBounds(space);
				drawGrids(space);
				if(config.imageSpaceDict[currentSpace]!=null && config.imageSpaceDict[currentSpace]!="None")
				{
					imageForOverlay.source=config.imageSpaceDict[currentSpace];
					bitmapData=drawImage(UIComponent(imageForOverlay),space);
				}
				drawAxes(space,bitmapData);
				if(config.IsEntityBoundsEnabled)
					drawEntityBounds(space);
				
				if(CanDisplayEntities)
				{
					entityArray=new Array();     
					drawGhostEntities(space);
					drawRealEntities(space);
				}				
				if(mode == ZOOM_SELECTING)
				{
					var rb:RubberBander=rubberBand;
					space.graphics.lineStyle(1,0,1.0,true,"normal",null,null,3.0);
					space.graphics.drawRect(rb.m_stpoint_x, rb.m_stpoint_y, rb.w, rb.h );
					rb=null;
				}
				//drawLoad(space);			
				bitmapData.draw(space,null,null,null,null,true);
				if(CanDisplayEntities)
				{
					drawRealIconEntities(bitmapData);
				}
				drawLoad(space1);
				drawAxes(space,bitmapData);
				drawCell(space1);
				drawSelectedCell(space1);
				
				
				//drawCellData(bitmapData);
				//drawLoad(space1);
				Application.application.entitieslbl.y=Application.application.svtile.y+this.height+25;
				Application.application.entitieslbl.text="Entities:"+realEntityData.length.toString();
				Application.application.ghostEntitieslbl.y=Application.application.svtile.y+this.height+25;
				Application.application.ghostEntitieslbl.text="Ghosts:"+ghostEntityData.length.toString();
				//finally draw bitmap data on the original image 
				bitmapData.draw(space1,null,null,null,null,true);
				drawCellData(bitmapData);
				this.source=new Bitmap(bitmapData);				
				//space1 = null;
			}
		}

		/**
		 * Function to draw the Entity bounds
		 */
		private function drawEntityBounds(space:Shape):void
		{
			space.graphics.lineStyle(0,config.EntityBoundsColour);
			for(var eI:int;eI<replayer.entityBounds.length;eI++)
			{
				var x1:int=transImage.TransformX(replayer.entityBounds[eI][0]);
				var x2:int=transImage.TransformX(replayer.entityBounds[eI][2]);
				var y1:int=transImage.TransformY(replayer.entityBounds[eI][1]);
				var y2:int=transImage.TransformY(replayer.entityBounds[eI][3]);
				space.graphics.drawRect(x1,y1,x2-x1,y2-y1);
			}
		}
		
		/**
		 * Function used for Image Overlay 
		 */
		private function drawImage( target : UIComponent,space:Shape) : BitmapData
		{
			var p1:int=transImage.TransformX(spaceBounds[SPACE_X1]);
			var p3:int=transImage.TransformX(spaceBounds[SPACE_X2]);
			var p2:int=transImage.TransformY(spaceBounds[SPACE_Y1]);
			var p4:int=transImage.TransformY(spaceBounds[SPACE_Y2]);
			var bd : BitmapData = new BitmapData(imageWidth,imageHeigth);
			var m : Matrix;
			m = new Matrix(((p3-p1)/target.width),0,0,((p2-p4)/target.height),p1,p4);
			bd.draw(target, m );
			
			m=null;    		//destroy matrix object
			return bd;
		}
		
		/**
		 * Function to draw space bounds.
		 */
		private function drawSpaceBounds(space:Shape):void 
		{
			var p1:int=transImage.TransformX(spaceBounds[SPACE_X1]);
			var p3:int=transImage.TransformX(spaceBounds[SPACE_X2]);
			var p2:int=transImage.TransformY(spaceBounds[SPACE_Y1]);
			var p4:int=transImage.TransformY(spaceBounds[SPACE_Y2]);
			
			space.graphics.lineStyle(1,config.SpaceBoundsColour,1.0,true,"normal",null,null,3.0);
			space.graphics.drawRect(p1,p4,(p3-p1),(p2-p4));
			//space.graphics.drawRect(transImage.TransformX(2),transImage.TransformY(2),20,20);
		}
		
		/**
		 * Function used to draw the grid lines
		 */
		 
		private function drawGrids(space:Shape):void
		{
			var worldX1:Number = Math.max( chunkBounds[0], spaceBounds[SPACE_X1] );
			var worldX2:Number = Math.min( chunkBounds[2], spaceBounds[SPACE_X2] );
			var worldY1:Number = Math.max( chunkBounds[1], spaceBounds[SPACE_Y1] );
			var worldY2:Number = Math.min( chunkBounds[3], spaceBounds[SPACE_Y2] );
			
		/*	# Convert to screen
			offset = -1
			screenX1 = max( -offset,              self.transformX( worldX1 ) )
			screenX2 = min( self.Width + offset,  self.transformX( worldX2 ) )
			screenY1 = max( -offset,              self.transformY( worldY2 ) )
			screenY2 = min( self.Height + offset, self.transformY( worldY1 ) )*/
			var x:int;
			var y:int;
			var gridResolution:Number=replayer.gridResolution;
			var screenX:int=Math.max(1,transImage.TransformX(worldX1));
			var screenY:int=Math.max(1,transImage.TransformY(worldY1));
			var screenX1:int=Math.min(transImage.width,transImage.TransformX(worldX2));
			var screenY1:int=Math.min(transImage.height,transImage.TransformY(worldY2));
			var screenX0:int;
			var screenY0:int;
			space.graphics.lineStyle(1,config.GridColour,1.0,true,"normal",null,null,3.0);
			//space.graphics.drawRect(transImage.TransformX(2),transImage.TransformY(2),20,20);
			for(x=worldX1+gridResolution;x<worldX2;x+=gridResolution)
			{
				screenX0=transImage.TransformX(x);
				space.graphics.moveTo(screenX0,screenY);
				space.graphics.lineTo(screenX0,screenY1);
			}
			for(y=worldY1+gridResolution;y<worldY2;y+=gridResolution)
			{
				screenY0=transImage.TransformY(y);
				space.graphics.moveTo(screenX,screenY0);
				space.graphics.lineTo(screenX1,screenY0);
			}
		}
		
		/**
		 * Function used to draw ghost entities
		 */
		private function drawGhostEntities(space:Shape):void 
		{
			var iterator:int;
			space.graphics.lineStyle(0);
			for(iterator=0;iterator<ghostEntityData.length;iterator++)
			{
				space.graphics.lineGradientStyle(GradientType.RADIAL ,[0x000000],null,null,null,"pad","rgb",0.0);
				space.graphics.beginFill(config.GhostEntityColour);
				space.graphics.drawCircle(transImage.TransformX(ghostEntityData[iterator][ENTITY_X]),transImage.TransformY(ghostEntityData[iterator][ENTITY_Y]),(Number(entitySize)*config.EntitySize));
				space.graphics.endFill();
			}
		}
		
		/**
		 * Function used to draw real entities
		 */
		private function drawRealEntities(space:Shape):void 
		{
			var iterator:int;			
			space.graphics.lineGradientStyle(GradientType.RADIAL ,[0X000000],null,null,null,"pad","rgb",0.0);
			space.graphics.lineStyle(0);			
			for(iterator=0;iterator<realEntityData.length;iterator++)
			{
				var type:String=typeNames[realEntityData[iterator][2]];
				var iterator2:int=0
				for(iterator2=0;iterator2<config.iconarray.length;iterator2++)
				{
					if(type==config.iconarray[iterator2].entity_type)
					{
						if(config.iconarray[iterator2].icon_path!="None")
						{
							break;
						}
					}
				}
				if(iterator2<config.iconarray.length)
				{
					continue;
				}
				var x:int=transImage.TransformX(realEntityData[iterator][ENTITY_X]);
				var y:int=transImage.TransformY(realEntityData[iterator][ENTITY_Y]);
				var tempentity:Array=new Array(x,y,int(realEntityData[iterator][ENTITY_TYPE]),realEntityData[iterator][ENTITY_ID],realEntityData[iterator][ENTITY_X],realEntityData[iterator][ENTITY_Y]);
				space.graphics.beginFill(0xFF0000);
				space.graphics.drawCircle(x,y,(Number(entitySize)*config.EntitySize));
				space.graphics.endFill();
				entityArray.push(tempentity);
			}
		}
		
		/**
		 * Function used to draw real entities as icons
		 */
		private function drawRealIconEntities(imgdata:BitmapData):void // take here arguments of the function as uid and spaceid
		{
			var iterator:int;
			var matrix:Matrix = new Matrix();                 
			
			for(iterator=0;iterator<realEntityData.length;iterator++)
			{
				var type:String=typeNames[realEntityData[iterator][2]];
				var iterator2:int=0;				
				for(iterator2=0;iterator2<config.iconarray.length;iterator2++)
				{
					if(type==config.iconarray[iterator2].entity_type)
					{
						if(config.iconarray[iterator2].icon_path!="None")
						{
							if(iconpaths[type])
							{
								//Alert.show(iconpaths[type].height.toString());
								iconpaths[type].height=config.EntitySize*16;
								iconpaths[type].width=config.EntitySize*16;	
								/*iconpaths[type].height=config.EntitySize*10;
								iconpaths[type].width=config.EntitySize*10;*/							
								var x:int=transImage.TransformX(realEntityData[iterator][ENTITY_X]);
								var y:int=transImage.TransformY(realEntityData[iterator][ENTITY_Y]);
								var tempentity:Array=new Array(x,y,int(realEntityData[iterator][ENTITY_TYPE]),realEntityData[iterator][ENTITY_ID],realEntityData[iterator][ENTITY_X],realEntityData[iterator][ENTITY_Y]);
								//matrix.tx=x-(config.EntitySize*10)/2;
								//matrix.ty=y-(config.EntitySize*10)/2;
								matrix.tx=x-iconpaths[type].width/2;
								matrix.ty=y-iconpaths[type].height/2;
								imgdata.draw(UIComponent(iconpaths[type]),matrix);							
								entityArray.push(tempentity);
							}
						}
						break;
					}
				}
			}
			matrix = null;	// Release the memory
		}
		
		/**
		 * Function used to draw the cell
		 */
		private function drawCell(space:Shape):void
		{
			var i:int;
			space.graphics.lineStyle(1,config.CellBoundaryColour,1.0,true,"normal",null,null,3.0);
			space.graphics.beginFill(0x0000FF);
			//lab.text=cellRects.length.toString();
			
			// cellBoundary consists of points to consecutive points defines 
			// cell boundary i.e to draw two cells it has 4 points length is 4
			for(i=0;i<cellBoundary.length;i++)
			{
				isSingleCell = false;
				var x1:int=transImage.TransformX(cellBoundary[i].x);
				var y1:int=transImage.TransformY(cellBoundary[i].y);
				var x2:int=transImage.TransformX(cellBoundary[i+1].x);
				var y2:int=transImage.TransformY(cellBoundary[i+1].y);
				var sx1:int=transImage.TransformX(spaceBounds[SPACE_X1]);
				var sx2:int=transImage.TransformX(spaceBounds[SPACE_X2]);
				var sy2:int=transImage.TransformY(spaceBounds[SPACE_Y1]);
				var sy1:int=transImage.TransformY(spaceBounds[SPACE_Y2]);
				
			
				space.graphics.moveTo(x1,y1);
				space.graphics.lineTo(x2,y2);
				// increment again i because we are using two points to draw
				i++;
			}
			if(i<=1)
				isSingleCell = true;
			space.graphics.endFill();
		}
		
		/**
		 * Function used to draw the Axis
		 */
		private function drawAxes(space:Shape,imgsrc:BitmapData):void
		{
			if(transImage.width!=0)
			{
				var	minPageDimension:int = Math.min(transImage.GetXPageSize(),transImage.GetYPageSize() );
				var exponent:Number = Math.floor( Math.log(minPageDimension*0.8 )/Math.log(10) );
				var dist:Number = Math.pow(10,exponent);
				var pixEquals:Number = 1
				if (transImage.width != 0)
					pixEquals = transImage.XPageSize / Number(transImage.width);
				var text1:TextField=new TextField();
				text1.width=200;
				var textformat:TextFormat = new TextFormat();
				textformat.font="Verdana";
				textformat.size=12;
				textformat.color=0x0000a0;
				text1.defaultTextFormat=textformat;
				var barLength:Number = dist / pixEquals;
				var constBarlenght:Number=100;
				//lab.text=barLength.toString();
				if (exponent >= 3.0)
				{
					text1.text = "1" ;
					//+ ((int(exponent) - 3) * "0") + "km"
					var i:int=(int(exponent) - 3);
					while(i>0)
					{
						text1.appendText("0");
						i--;
					}
					
					//text1.appendText("km");
				}
				else if (exponent >= 0.0)
				{
					text1.text = "1" 
					//+ (int(exponent) * "0") + "m"
					var j:int=(int(exponent));
					while(j>0)
					{
						text1.appendText("0");
						j--;
					}
					//text1.appendText("m");
				}
				else
				{
					text1.text = "0."
					// + ((-1 - int(exponent)) * "0") + "1m"
					var k:int=(-1-int(exponent));
					while(k>0)
					{
						text1.appendText("0");
						k--;
					}
					text1.appendText("1");
					//text1.appendText("1m");
				}
				var no:Number=Number(text1.text);
				no=Math.round((no*constBarlenght/barLength));
				if(no.toString().length>1)
				{
					//text1.text = (5*Math.pow(10,(no.toString().length-2))).toString();
					if(no%(Math.pow(10,(no.toString().length-1)))>(5*Math.pow(10,(no.toString().length-2))))
					{
						no=(int(no/(Math.pow(10,(no.toString().length-1))))+1)*Math.pow(10,(no.toString().length-1));	
					}
					else
					{
						no=int(no/(Math.pow(10,(no.toString().length-1))))*Math.pow(10,(no.toString().length-1));
					}
					text1.text = no.toString();
				}
				else
					//text1.text=Math.round((no*constBarlenght/barLength)).toString();
					text1.text=no.toString();
					
				if(no.toString() == "0")
				{
					text1.text = "1";
				}
				if (exponent >= 3.0)
				{
					text1.appendText("km");
				}
				else
				{
					text1.appendText("m");
				}
				/*if(exponent >= 3.0)
				{
					text1.text=Math.round((no*constBarlenght/barLength)).toString()+"km";
				}
				else
				{
					text1.text=Math.round((no*constBarlenght/barLength)).toString()+"m";
				}*/
				text1.text= "Scale : "+text1.text;
				space.graphics.lineStyle(2,0x000000,1.0,true,"normal",null,null,3.0);
				space.graphics.moveTo(40,transImage.height-35);
				space.graphics.lineTo(40,transImage.height-30);
				//space.graphics.lineTo(barLength+40,transImage.height-30);
				space.graphics.lineTo(constBarlenght+40,transImage.height-30);
				//space.graphics.lineTo(200,transImage.height-30);
				//space.graphics.moveTo(barLength+40,transImage.height-30);
				//space.graphics.moveTo(200,transImage.height-30);
				//space.graphics.lineTo(barLength+40,transImage.height-35);
				space.graphics.lineTo(constBarlenght+40,transImage.height-35);
				//space.graphics.lineTo(200,transImage.height-35);
				
				var matrix:Matrix=new Matrix(1,0,0,1,50,transImage.height-50);
				imgsrc.draw(text1,matrix);
				
				space.graphics.moveTo(14,transImage.height-14);
				
				//Release the memory
				text1 		= null;
				textformat 	= null;
				/*mat 		= null;
				mat1 		= null;*/
				matrix      = null;
			}
		}
		
		/**
		 * Function to draw the Selected Cell
		 */
		private function drawSelectedCell(space:Shape):void
		{
			/*var x1:Number = transImage.TransformX(cellRect[0] );
			var x2:Number = transImage.TransformX( cellRect[2] );
			var y1:Number = transImage.TransformY( cellRect[1] );
			var y2:Number= transImage.TransformY(cellRect[3] );*/
			var x1:Number = transImage.TransformX(cellRects[cellRect[4]].minX );
			var x2:Number = transImage.TransformX( cellRects[cellRect[4]].maxX );
			var y1:Number = transImage.TransformY( cellRects[cellRect[4]].minY );
			var y2:Number= transImage.TransformY(cellRects[cellRect[4]].maxY );
			
			Application.application.cellIDlbl.y=Application.application.svtile.y+this.height+25;
			Application.application.cellIDlbl.text="Cell:"+cellRect[4].toString();
			Application.application.cellIPlbl.y=Application.application.svtile.y+this.height+25;
			Application.application.cellIPlbl.text="IP:"+cellRect[5].toString();
			Application.application.cellLoadlbl.y=Application.application.svtile.y+this.height+25;
			Application.application.cellLoadlbl.text="Load:"+Number(cellRect[6]).toFixed(3);
			
			if(isSingleCell == false){
			try
			{
				
			var newX1:Number = Math.max( x1, -3 );
			var newY1:Number = Math.min( y1, transImage.height+10);
			var newX2:Number=Math.min(x2,transImage.width+10);
			var newY2:Number = Math.max( y2, -3);
			//Alert.show("x1  :"+newX1.toString()+" y1:"+newY1.toString()+"  width:"+(newX2 - newX1).toString()+" height:"+(newY2 - newY1).toString());
			
			
			var oldX1:Number= Math.max( x1, 0 );
			var oldY1:Number= Math.min( y1, transImage.height);
			var oldX2:Number= Math.min(x2,transImage.width-50);
			var oldY2:Number= Math.max(y2,0);
			//lab.text="x1  :"+oldX1.toString()+" y1:"+y1.toString()+"  width:"+(oldX2 - oldX1).toString()+" height:"+(oldY2 - oldY1).toString();
			var dashLine:DashedLine=new DashedLine();
			dashLine.drawDashedLine(space,oldX1,oldY1,oldX2,oldY1,5,2,config.CellBoundaryColour);
			dashLine.drawDashedLine(space,oldX2,oldY1,oldX2,oldY2,5,2,config.CellBoundaryColour);
			dashLine.drawDashedLine(space,oldX2,oldY2,oldX1,oldY2,5,2,config.CellBoundaryColour);
			dashLine.drawDashedLine(space,oldX1,oldY2,oldX1,oldY1,5,2,config.CellBoundaryColour);
			space.graphics.lineStyle(3,config.CellBoundaryColour);
			space.graphics.drawRect(newX1,newY1,newX2 - newX1,newY2 - newY1);
			}catch(ex:Error)
			{
			
			}
			}		
		}
		
		/**
		 * Function to draw the Load Indicator
		 */
		public function drawLoad(space:Shape):void
		{
			//lab.text="";
			var index:int = 0;
			var perceivedload:Number = 0.0;
			var rgb:Number;
			var colourObj:Colour     = new Colour();
			if(config.IsRelativeColour == true)
			{
				var minload:Number; 
				var maxload:Number;
				
				var loads:Array          = new Array();
				for(index=0;index<cellDataLoads.length;index++)
				{
					loads.push(cellDataLoads[index]);
				}
				loads=loads.sort();
				minload=loads[0];
				maxload=loads[cellDataLoads.length-1];
				for(index=0;index<loads.length;index++)
				{
					loads.pop();
				}
				loads = null;
			}
			for(var i:int=0;i<cellDataIds.length;i++)
			{
				var xMin:Number = Math.max( 0,transImage.TransformX( cellRects[cellDataIds[i]].minX ) );
				var yMin:Number = Math.max( 0, transImage.TransformY( cellRects[cellDataIds[i]].maxY ) );
				var xMax:Number = Math.min( transImage.width, transImage.TransformX(cellRects[cellDataIds[i]].maxX) );
				var yMax:Number = Math.min( transImage.height,transImage.TransformY(cellRects[cellDataIds[i]].minY) );
				var x:int = (xMin + xMax)/2;
				var y:int = (yMin + yMax)/2;
				/*var matrix:Matrix = new Matrix();
				matrix.tx=x-25;
				matrix.ty=y-17;*/
				if(cellDataIds[i].toString()==cellRect[4].toString())
				{
					space.graphics.beginFill(0x68a3ea);
				}
				else
				{
					space.graphics.beginFill(0xc0c0c0);
				}
				space.graphics.drawRoundRect(x-30,y-15,60,25,7,7);
				space.graphics.endFill();
				
				space.graphics.lineStyle(0);
				space.graphics.lineGradientStyle("linear",[0X000000],[0.5],null);
				
				if(cellDataLoads[i]>=1.0)
				{
					if(config.IsRelativeColour == true)
					{
						if(cellDataIds.length>1)
						{
							perceivedload = (cellDataLoads[i] - minload) /(maxload - minload);
						}
						else
						{
							perceivedload = 0.5;
						}
						rgb = colourObj.HSV2RGB( (1-perceivedload)/2.0, 1, 1 );
						space.graphics.beginFill(rgb);
					}
					else
					{
						//perceivedload = Math.min( cellDataLoads[index], MAX_SERVER_CPU ) / MAX_SERVER_CPU;
						//rgb = colourObj.HSV2RGB( (1-perceivedload)/2.0, 1, 1 );						
						space.graphics.beginFill(config.CellLoadColour);
					}
					space.graphics.drawRect(x-25,y,50,5);
					space.graphics.endFill();
				}
				else
				{
					var x1:int=x+(cellDataLoads[i]/1.0)*50; 
					//lab.text+=cellDataLoads[i].toString();
					if(config.IsRelativeColour == true)
					{
						if(cellDataIds.length>1)
						{
							perceivedload = (cellDataLoads[i] - minload) /(maxload - minload);
						}
						else
						{
							perceivedload = 0.5;
						}
						rgb = colourObj.HSV2RGB( (1-perceivedload)/2.0, 1, 1 );
						space.graphics.beginFill(rgb);
					}
					else
					{
						perceivedload = Math.min( cellDataLoads[i], MAX_SERVER_CPU ) / MAX_SERVER_CPU;
						rgb = colourObj.HSV2RGB( (1-perceivedload)/2.0, 1, 1 );
						space.graphics.beginFill(rgb);
					}
					
					space.graphics.drawRect(x-25,y,x1-x,5);                
					space.graphics.endFill();
					space.graphics.beginFill(0xFFFFFF);
					space.graphics.drawRect(x1-25,y,50-x1+x,5);
					space.graphics.endFill();					                
				}
			}			
		}

		/**
		 * Function to draw the Cell data such as Space ID ,
		 *  IP Address and Partition Load 
		 */
		private function drawCellData(imgsrc:BitmapData):void
		{
			var i:int=0;
			for(i=0;i<cellDataIds.length;i++)
			{
				var xMin:Number = Math.max( 0,transImage.TransformX( cellRects[cellDataIds[i]].minX ) );
				var yMin:Number = Math.max( 0, transImage.TransformY( cellRects[cellDataIds[i]].maxY ) );
				var xMax:Number = Math.min( transImage.width, transImage.TransformX(cellRects[cellDataIds[i]].maxX) );
				var yMax:Number = Math.min( transImage.height,transImage.TransformY(cellRects[cellDataIds[i]].minY) );
				var x:int = (xMin + xMax)/2;
				var y:int = (yMin + yMax)/2;
				var matrix:Matrix = new Matrix();
				matrix.tx=x-25;
				matrix.ty=y-17;
												
				var textformat:TextFormat = new TextFormat();
				textformat.bold=true;
				textformat.font="Verdana";
				textformat.color=config.CellAppIDColour;
				textformat.size=11;
				
				var textID:TextField = new TextField();
				textID.defaultTextFormat=textformat;
				textID.text="Cell "+cellDataIds[i].toString();
				textID.text.toUpperCase();
				imgsrc.draw(textID,matrix,null,null,null,false);
				
				
			/*
			if(config.IsPartitionLoadEnabled) //if partition load is enabled then show the partition load
			{
				var z:int=0;
				var l:int=0;
				for(z=0, l=0;z<cellBoundary.length;z++,l++)
				{
					var x1:int=transImage.TransformX(cellBoundary[z][0]);
					var y1:int=transImage.TransformY(cellBoundary[z][1]);				
					var x2:int=transImage.TransformX(cellBoundary[z+1][0]);
					var y2:int=transImage.TransformY(cellBoundary[z+1][1]);
					var sx1:int=transImage.TransformX(spaceBounds[SPACE_X1]);
					var sx2:int=transImage.TransformX(spaceBounds[SPACE_X2]);
					var sy2:int=transImage.TransformY(spaceBounds[SPACE_Y1]);
					var sy1:int=transImage.TransformY(spaceBounds[SPACE_Y2]);
					if(x1<Math.min(sx1,sx2))
						x1=3;
					if(x1>Math.max(sx1,sx2))
						x1=transImage.width-3;
					if(x2<Math.min(sx1,sx2))
						x2=3;
					if(x2>Math.max(sx1,sx2))
						x2=transImage.width-3;
					if(y1<Math.min(sy1,sy2))
						y1=0;
					if(y1>Math.max(sy1,sy2))
						y1=transImage.height-3;
					if(y2<Math.min(sy1,sy2))
						y2=3;
					if(y2>Math.max(sy1,sy2))
						y2=transImage.height-3;
					
					var matrix2:Matrix = new Matrix();
					matrix2.tx=(x1+x2)/2;
					matrix2.ty=(y1+y2)/2;
					            	
					//this part is to display partition load
					var text3:TextField = new TextField();
					var textformat:TextFormat = new TextFormat();
					textformat.bold=true;
					textformat.color=config.PartitionLoadColour;
					text3.defaultTextFormat=textformat;
					text3.text=Number(replayer.branchLoads[0]).toFixed(3);

					imgsrc.draw(text3,matrix2,null,null,null,false);
					z++;

					// Release the memory
					matrix2 = null;
					text3 	= null;
				}*/
			}
		}
		
		/**
		 * Function for Zoom In or Zoom Out the Image depending upon the ZoomImage flag
		 * ZoomImage = True : Zoom out
		 * ZoomImage = Flase : Zoom In
		 */
		public function ZoomImage(zoomFlag:Boolean):void
		{
			if((zoomFlag==false && slider.value<=SliderMaxValue)||(zoomFlag==true && slider.value<SliderMaxValue))
			{
				if(!zoomFlag)
				{
					if(slider.value>SliderMinValue && transImage.XPageSize>=transImage.OrigXPageSize/SliderMaxValue)
						slider.value-=(SliderMaxValue-SliderMinValue)/8.0;
				}
				else
				{
					if(slider.value<SliderMaxValue && transImage.XPageSize<=transImage.OrigXPageSize)
						slider.value+=(SliderMaxValue-SliderMinValue)/8.0;
				}
				transImage.ZoomImage( zoomFlag );
				replayer.timerHandler();
				Refresh();
			}
		}
		
		/**
		 * Function to Zoom the image upto given magnitude "mag"
		 */
		public function Zoom(x:int, y:int, mag:Number ):void
		{
			transImage.Zoom(x, y, mag);
			replayer.timerHandler();
			Refresh();
		}
		
		/**
		 * Function to do Zoom to Space Bounds
		 */
		public function zoomToBounds():void
		{
			if(replayer.spaceBounds!=null)
			{
				spaceBounds=replayer.spaceBounds;
				transImage.ZoomToSpaceBounds(spaceBounds);
				transImage.IsZoomed=false;
				replayer.timerHandler();
				slider.value=SliderMinValue;
				flag=0;
			}
		}
		
		/**
		 * Slider Change Event Handler to Zoom In or Zoom Out the Space
		 */
		public function sliderChange():void
		{
			if(flag==0)
			{
				xPos=transImage.OrigXPos;
				yPos=transImage.OrigYPos;
				flag=1;
			}
			transImage.ZoomToSpaceBounds(spaceBounds);
			transImage.XPosition=xPos;
			transImage.YPosition=yPos;
			transImage.Zoom(transImage.width*0.5,transImage.height*0.5,slider.value);
			//Alert.show(transImage.XPageSize.toString());
			replayer.timerHandler();
			Refresh();
		}
		
		/**
		 * This method contain clean up of the memory 
		 */
		public function onDestroy():void
		{
			this.removeEventListener(KeyboardEvent.KEY_DOWN,onKeyDown);
			this.removeEventListener(MouseEvent.MOUSE_DOWN,onLMouseDown);
			this.removeEventListener(MouseEvent.MOUSE_MOVE,onMouseMove);
			this.removeEventListener(MouseEvent.MOUSE_UP,onLMouseUp);
			this.removeEventListener(MouseEvent.MOUSE_WHEEL,onMouseWheel);
			timerDraw.removeEventListener(TimerEvent.TIMER,OnTimer);
			//Application.application.removeChild(arrowDown);
			//Application.application.removeChild(arrowRight);
			Application.application.removeChild(imageForOverlay);
			// deletion of the iconArray dictionary elements		
			for(var i:int=0;i<config.iconarray.length;i++)
			{
				if(config.iconarray[i].icon_path!="None")
				{
					Application.application.removeChild(iconpaths[config.iconarray[i].entity_type]);
					delete iconpaths[config.iconarray[i].entity_type];
				}
			}

			replayer.onDestroy();
			transImage			=	null;
			timerDraw			=	null;
			spaceBounds			=	null;
			realEntityData		=	null;
			ghostEntityData		=	null;
			cellBoundary		=	null;
			replayer			=	null;
			cellRect			=	null;
			cellRects			=	null;
			cellDataIds			=	null;
			cellDataIps			=	null;
			cellDataLoads		=	null;
			typeNames			=	null;
			entityArray			=	null;
			movementSave		=	null;
			imageForOverlay		=	null;
			tooltipArray		=	null;
			config				=	null;
			slider				=	null;
			space				=	null;
			space1				=	null;
			rubberBand			=	null;
			chunkBounds			=	null;
			iconpaths           =   null;
			
			bitmapData.dispose();
			//bitmapData 			=	null;
		}
	}
}
