package
{
	import mx.containers.Panel;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import mx.controls.Alert;
	import mx.controls.Label;
	import mx.controls.Image;
	import mx.controls.HSlider;
		
	/**
	 * This represents the viewer for a particular space.
	 * 
	 */	
	public class SVPanel extends Panel
	{
		private var state:int = 0;		
		private var boolSized:Boolean = false;
		
		private var bMaximized:Boolean = false;
		
		private var FullHeight:uint=580;
		private var FullWidth:uint=980;
		
		private var NormalHeight:uint;
		private var NormalWidth:uint;
		
		//private var lblTitle:Label;
		private var imgSpace:SVImage;
		
		private var strUser:String;
		private var strSpace:String;
		
		private var config:ConfigurationData;		
		
		private var slider:HSlider;
		
		[Event(name="restore")]
		[Event(name="maximize")]

		public function SVPanel(user:String, space:String,configTemp:ConfigurationData,sliderTemp:HSlider)
		{
			strUser = user;
			strSpace = space;
			config=configTemp;
			slider=sliderTemp;
			
			//this.color=="#400040";
			//this.width=FullWidth;
			//this.height=FullHeight;
			this.layout="absolute";
			
			super();
		}
		
		public function get IsMaximized():Boolean
		{
			return bMaximized;
		}
		
		public function set IsMaximized(bFlag:Boolean):void
		{
			bMaximized = bFlag;
		}
		
		public function SetSize(nWidth:uint, nHeight:uint):void
		{
			this.width = nWidth;
			this.height = nHeight;
			NormalHeight = nHeight;
			NormalWidth = nWidth;
			imgSpace.SetSize(nWidth, nHeight);
		}
		
		public function SetTitle(strTitle:String):void
		{
			// This needs to be done, this is used to 
			// identify the panel by title.
			this.title = strTitle;
			//lblTitle.text = strTitle;
		}
		
		public function SetSVStyle(style:uint):void
		{
			if(screen.width>300)
				FullWidth  = screen.width-30;
			else
				FullWidth  = 200;
			
			if(screen.height>300)
				FullHeight = screen.height-60;
			else
				FullHeight = 200;
			
			NormalHeight = FullHeight/style;
			NormalWidth  = FullWidth /style;
			
			SetSize(NormalWidth, NormalHeight);
			
			if(style == SpaceViewerStyles.ONE_X_ONE)
			{
				IsMaximized = true;
			}
			else
			{
				IsMaximized = false;
			}
		}
		
		protected override function createChildren():void
		{
			super.createChildren();
			
			//lblTitle = new Label();
			// width="348" fontSize="12" fontFamily="Verdana" color="#0000a0" textAlign="center" x="10" y="10"
			//addChild(lblTitle);        	
			
			imgSpace = new SVImage(strUser, strSpace, this.width, this.height,config,slider);
			imgSpace.setStyle("bottom","0");
			imgSpace.setFocus();
			imgSpace.visible = true;
			
			addChild(imgSpace);
			addEventListener(MouseEvent.MOUSE_DOWN,ShowFocus);
		}
		
		protected override function updateDisplayList(unscaledWidth:Number, unscaledHeight:Number):void
		{
			super.updateDisplayList(unscaledWidth, unscaledHeight);
			if(unscaledWidth >0)
			{
				this.visible = true;
			}
			else
			{
				this.visible = false;
			}
			
			/*lblTitle.width = this.width - 40;
			lblTitle.height = 20;
			lblTitle.move(0,5);
			lblTitle.setStyle("fontSize","12");
			lblTitle.setStyle("fontFamily","Verdana");
			lblTitle.setStyle("textAlign","center");
			lblTitle.setStyle("color","#0000a0");
			lblTitle.visible = true;*/
		}
		
		public function ShowFocus(e:MouseEvent):void
		{
			e.currentTarget.drawFocus(true);
		}
		
		private function maximizePanel():void
		{
			this.height = FullHeight;
			this.width = FullWidth;
				
			imgSpace.SetSize(this.width, this.height);
				
			IsMaximized = true;
				
			// For Debuging
			// this.title = "Maxi H :" + FullHeight.toString() + " W : " + FullWidth.toString();
		}

		private function restorePanel():void
		{
			this.height = NormalHeight;
			this.width  = NormalWidth;
			
			imgSpace.SetSize(this.width, this.height);
			
			IsMaximized = false;
			
			// For Debugging
			// this.title = "Restore H :" + NormalHeight.toString() + " W : " + NormalWidth.toString();
		}
		
		public function doMaximize():void
		{
			setState(1);
			maximizePanel();
		}
		
		public function doRestore():void
		{
			setState(0);
			restorePanel();
		}
		
		private function setState(state:int):void
		{
			this.state=state;
			if (state==0)
			{ // Minimized
				this.dispatchEvent(new Event('restore'));
			}
			else
			{
				this.dispatchEvent(new Event('maximize'));
			}
		}
		
		/**Function for Zoom In or Zoom Out the Image depending upon the ZoomImage flag
		 * ZoomImage = True : Zoom out
		 * ZoomImage = False : Zoom In
		 * actual functionality is done in svimage
		 */	
		public function ZoomImage(zoomFlag:Boolean):void
		{
			imgSpace.ZoomImage( zoomFlag );
		}
		
		/**
		 * Function to do Zoom to Space Bounds
		 * actual functionality down in svimage class this calls it
		 */
		public function zoomToBounds():void
		{
			imgSpace.zoomToBounds();
		}
		
		/**
		 * Function to Zoom the image upto given magnitude "mag"
		 * actual functionality down in svimage class this calls it
		 */
		public function Zoom(x:int, y:int, mag:Number):void
		{
			imgSpace.Zoom(x, y, mag);
		}
		
		public function SetDisplayEntities(bFlag:Boolean):void
		{
			imgSpace.DisplayEntities = bFlag;
		}
		
		/**
		 * Slider Change Event Handler to Zoom In or Zoom Out the Space
		 * actual functionality down in svimage class this calls it
		 */
		public function sliderChange():void
		{
			imgSpace.sliderChange();
		}
		
		/**
		 * This method contain clean up of the memory 
		 */
		public function onDestroy():void
		{
			removeEventListener(MouseEvent.MOUSE_DOWN,ShowFocus);
			imgSpace.onDestroy();
			//lblTitle	=	null;
			imgSpace	=	null;
			strUser		=	null;
			strSpace	=	null;
			config		=	null;
			slider		=	null;
		}
	}
}