package
{
	import mx.containers.Tile;
	import mx.collections.ArrayCollection;
	import flash.events.MouseEvent;
	import mx.controls.Alert;
	import mx.controls.HSlider;
	import mx.core.Application;
	
	public class SVTile extends Tile
	{
		private var SVPanelArray:ArrayCollection;
		private var strUser:String;
		private var strSpace:String;
		private var config:ConfigurationData;
		private var slider:HSlider;
		
		public  function set enable(set1:Boolean):void
		{
			this.enabled=set1;
		}
		
		public function SVTile()
		{
			super();
			this.autoLayout = true;
			SVPanelArray = new ArrayCollection();	
		}
		
		public function onResize():void
		{
			if(SVPanelArray.length>0)
			{
				SetSVStyle(1);							
			}						
		}
		
		public function CreateSpaceViewerPanel(strTitle:String,configTemp:ConfigurationData,sliderTemp:HSlider):uint
		{
			// do close svpanel if any
			// this can be removed if we want to display multiple svpanels
			doClose();
			var config:ConfigurationData=configTemp;
			slider=sliderTemp;
			strSpace = strTitle.charAt(strTitle.length - 1);
			// Reset all panels to normal view before creating new one.
			ResetAllPanels();
			
			var nIndex:int = IsSVPanelAlreadyCreated(strTitle);
			
			// For debugging
			// var nIndex:int = -1;
			if(nIndex != -1)
			{
				SVPanelArray[nIndex].drawFocus(true);
				// Returning the index of already created svpanel
				return nIndex;
			}
			
			var space:SVPanel = new SVPanel(strUser, strSpace,config,slider);
			space.visible = true;
			space.setStyle("headerHeight","0");
			//space.setStyle("borderStyle","solid");
			/*space.setStyle("fontFamily","Verdana");
			space.setStyle("fontSize","12");
			space.setStyle("color","#ffffff");
			space.setStyle("themeColor","#ff8000");
			
			
			space.setStyle("borderThicknessRight","0");
			space.setStyle("borderThicknessBottom","0");
			space.setStyle("horizontalGap","0");
			space.setStyle("borderThickness", "1");	*/		
			space.addEventListener(MouseEvent.DOUBLE_CLICK, doMaximize);
			
			SVPanelArray.addItem(space);
			addChild(space);		
			
			// Setting the title for the svpanel
			space.SetTitle(strTitle);
			
			SetSVStyle(SVPanelArray.length);
			
			// Release the memory
			config = null;
			
			// Returning the index of newly created svpanel
			return SVPanelArray.length - 1;
		}
		
		public function SetUser(str:String):void
		{
			strUser = str;
		}
		
		private function IsSVPanelAlreadyCreated(strTitle:String):int
		{
			for(var i:uint=0; i<SVPanelArray.length; i++)
			{
				var svp:SVPanel = SVPanel(SVPanelArray.getItemAt(i));
				if(strTitle == svp.title)
				{
					//svpanel is already created.
					return i;
				}
			}
			return -1;
		}
		
		private function SetSVStyle(len:uint):void
		{
			var style:int = CalculateSVStyle(len);
			
			for(var i:uint=0; i<len; i++)
			{
				SVPanelArray[i].SetSVStyle(style);
			}
		}
		
		private function CalculateSVStyle(len:uint):int
		{
			if(len==1)
			{
				return SpaceViewerStyles.ONE_X_ONE;
			}
			
			if(len<=4)
			{
				return SpaceViewerStyles.TWO_X_TWO;
			}
			
			if(len<=9)
			{
				return SpaceViewerStyles.THREE_X_THREE;
			}
			if(len<=16)
			{
				return SpaceViewerStyles.FOUR_X_FOUR;
			}
			if(len<=25)
			{
				return SpaceViewerStyles.FIVE_X_FIVE;
			}
			return SpaceViewerStyles.FIVE_X_FIVE;
		}
		
		public function doMaximize(e:MouseEvent):void
		{
			var currIndex:int = GetPanelIndex(e.currentTarget);
			
			if(currIndex == -1)
				return;
			
			if(SVPanelArray[currIndex].IsMaximized == true)
			{
				SVPanelArray[currIndex].doRestore();
				RestoreOthersPanels(currIndex);
			}
			else
			{
				SVPanelArray[currIndex].doMaximize();
				HideOtherPanels(currIndex);
			}
		}
/*		
		public function doRestore(e:MouseEvent):void
		{
			var currIndex:int = GetPanelIndex(e.currentTarget);
			
			if(currIndex == -1)
				return;
				
			SVPanelArray[currIndex].doRestore();
			
			RestoreOthersPanels(currIndex);
		}
		
		public function doClose(e:MouseEvent):void
		{
			var currIndex:int = GetPanelIndex(e.currentTarget);
			
			if(currIndex == -1)
				return;
			
			SVPanelArray[currIndex].drawFocus(false);
			this.removeChild(SVPanelArray[currIndex]);
			SVPanelArray.removeItemAt(currIndex);
			
			//Setting the focus.
			if(currIndex != 0)
			{
				SVPanelArray[currIndex - 1].drawFocus(true);	
			}
						
			//Set the style again
			SetSVStyle(SVPanelArray.length);
		}
*/

		/**
		 * Used to close the existing svpanel, so that only one svpanel can
		 * be displayed at a time.
		 */
		public function doClose():void
		{
			if(SVPanelArray.length == 0)
				return;
			
			onDestroy();
			
			var currIndex:int = 0; // GetPanelIndex(e.currentTarget);
			
			if(currIndex == -1)
				return;
			
			SVPanelArray[currIndex].drawFocus(false);
			this.removeChild(SVPanelArray[currIndex]);
			SVPanelArray.removeItemAt(currIndex);
			
			// Setting the focus.
			if(currIndex != 0)
			{
				SVPanelArray[currIndex - 1].drawFocus(true);	
			}
						
			// Set the style again
			SetSVStyle(SVPanelArray.length);
		}
		
		public function ResetAllPanels():void
		{
			var currIndex:int = -1;
			
			for(var i:uint=0; i<SVPanelArray.length; i++)
			{
				if(SVPanelArray[i].IsMaximized)
				{
					currIndex = i;
					break;
				}
			}
			
			if(currIndex == -1)
				return;
			
			SVPanelArray[currIndex].doRestore();
			
			RestoreOthersPanels(currIndex);
		}
		
		private function GetPanelIndex(currTarget:Object):int
		{
			for(var i:uint=0; i<SVPanelArray.length; i++)
			{
				var svp:SVPanel = SVPanel(SVPanelArray.getItemAt(i));
				//if(svp.owns(DisplayObject(currTarget)))
				if(svp == currTarget)
					return i;
			}
			return -1;		
		}
		
		/**
		 * Remove the panels from the svtile
		 */
		private function HideOtherPanels(currIndex:int):void
		{
			for(var i:uint=0; i<SVPanelArray.length; i++)
			{
				if( i != currIndex)
					this.removeChild(SVPanelArray[i]);
				else
					SVPanelArray[i].drawFocus(true);
			}
		}
		
		/**
		 * Add the panels back to the svtile
		 */ 
		private function RestoreOthersPanels(currIndex:int):void
		{
			for(var i:uint=0; i<SVPanelArray.length; i++)
			{
				if( i != currIndex)
					this.addChildAt(SVPanelArray[i],i);
				else
					SVPanelArray[i].drawFocus(true);
			}
		}
		
		/**
		 * True : Zoom out
		 * False : Zoom In
		 */
		public function ZoomImage(zoomFlag:Boolean):void
		{
			// Method incomplete for multiple panels.
			// This will work only for single panel.
			var currIndex:int = 0;
			SVPanelArray[currIndex].ZoomImage(zoomFlag);
		}
		
		public function zoomToBounds():void
		{
			var currIndex:int = 0;
			SVPanelArray[currIndex].zoomToBounds();
		}
		
		private function Zoom(x:int, y:int, mag:Number):void
		{
			// Method incomplete for multiple panels.
			// This will work only for single panel.
			var currIndex:int = 0;
			SVPanelArray[currIndex].Zoom(x, y, mag);
		}
		
		public function SetDisplayEntities(bFlag:Boolean):void
		{
			// Method incomplete for multiple panels.
			// This will work only for single panel.
			var currIndex:int = 0;
			SVPanelArray[currIndex].SetDisplayEntities(bFlag);
		}
		
		public function sliderChange():void
		{
			var currIndex:int = 0;
			SVPanelArray[currIndex].sliderChange();
		}
		
		public function onDestroy():void
		{
			var index:uint;
			for(index=0;index<SVPanelArray.length;index++)
			{
				SVPanelArray[index].onDestroy();
			}
			config	=	null;
			slider	=	null;
		}
	}
}