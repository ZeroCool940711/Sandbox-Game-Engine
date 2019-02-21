package
{
	import mx.controls.Alert;
	
	/**
	 * This Class converts from world to image coordinates
	 * and vice versa.
	 */	
	public class Transformation
	{
		private var Width:Number=592;
		private var Height:Number=523;
		
		public var XPosition:Number=0;
		public var YPosition:Number=0;
		
		public var XPageSize:Number=0;
		public var YPageSize:Number=2;
		
		public var OrigXPageSize:Number;
		public var OrigXPos:Number;
		public var OrigYPos:Number;
		
		private const MAX_PAGE_SIZE:Number=260000.0;
		
		private var hasZoomed:Boolean = false;
		
		public function get width():Number
		{
			return Width;
		}
		
		public function set width(w:Number):void
		{
			Width=w;
		}
		
		public function get height():Number
		{
			return Height;
		}
		
		public function set height(h:Number):void
		{
			Height=h;
		}
		
		public function get IsZoomed():Boolean
		{
			return hasZoomed;
		}
		
		public function set IsZoomed(zoomFlag:Boolean):void
		{
			hasZoomed = zoomFlag;
		}
		
		public function SetSize(w:Number, h:Number):void
		{
			width = w;
			height = h;
		}
		
		/** 
		 * This function takes real world distance x in meters and returns 
		 * our application measurement (with respect image height and width)
		 */	
		public function TransformX(x:Number):Number
		{
			return Number((Number(x)-XPosition)/GetXPageSize() * Width );
		}
		
		/** 
		 * This function takes real world distance y in meters and returns 
		 * our application measurement (with respect image height and width)
		 */
		public function TransformY(y:Number):Number
		{
			return Number( (YPosition-Number(y))/GetYPageSize() * Height )
		}
		
		/** 
		 * This function takes x as in our application measurement (with respect image height and width)
		 * and returns real world distance x in meters 
		 */
		public function iTransformX(x:Number):Number
		{
			return (Number(x)/Number(Width) * GetXPageSize() + XPosition);
		}
		
		/** 
		 * This function takes y as in our application measurement (with respect image height and width)
		 * and returns real world distance y in meters 
		 */
		public function iTransformY(y:Number):Number
		{
			//Alert.show(GetYPageSize().toString()+"  and  "+YPosition.toString());
			return (YPosition - (Number(y))/Number(Height) * GetYPageSize());
		}
		
		public function GetXPageSize():Number
		{
			return XPageSize;
		}
		
		public function SetXPageSize(l:Number ):void
		{
			XPageSize = l;
		}
		
		public function GetYPageSize():Number
		{
			if(Width != 0)
				return (Number(XPageSize) * Number(Height) /Number(Width));
			else
				return XPageSize;
		}
		public function SetYPageSize(l:Number ):void
		{
			if(Height == 0)
				XPageSize = l;
			else
				XPageSize = Width /Height * l;
		}
		
		/**
		 *
		 * This function zooms the space to specified rectangle area.
		 */
		public function ZoomToRect(x1:Number, y1:Number, x2:Number, y2:Number ):void
		{
			var xw:Number = x2 - x1
			var yw:Number = y1 - y2
			var nyw:Number,nxw:Number;
			var ywdiff:Number,xwdiff:Number;
			
			if (xw != 0 && yw != 0)
			{
				XPosition = x1
				YPosition = y1
				
				//YPageSize = xw
			
				if ( xw / Width > yw /Height )
				{
					XPageSize = xw
					nyw = -iTransformY(Height ) + YPosition
					ywdiff = nyw - yw
					YPosition += ywdiff / 2.0
				}
				else
				{
					if(Height != 0)
					{
						XPageSize =Number(yw) * Number(Width) / Number(Height)
						nxw =iTransformX(Width ) - XPosition
						xwdiff = nxw - xw
						XPosition -= xwdiff / 2.0
					}
				}
			}
		}
		
		/**
		 * This functions zooms the space to space bounds
		 */
		public function ZoomToSpaceBounds(bounds:Array):void
		{
			//Alert.show("inside transform"+bounds.toString(),"dd");
			var width:Number = bounds[3] - bounds[0];
			var height:Number = bounds[5] - bounds[2];
			if ((width > 0)&&(height > 0))
			{
				var bufferRatio:Number = 0.1;
				var xBuf:Number= width * bufferRatio;
				var yBuf:Number = height * bufferRatio;
				ZoomToRect( bounds[0] - xBuf, bounds[5] + yBuf,
					bounds[3] + xBuf, bounds[2] - yBuf );
			}	
			OrigXPageSize=XPageSize;
			OrigXPos=XPosition;
			OrigYPos=YPosition;
		}
		
		/**
		 * This function is responsible for Zooming In (doubled) or Zooming Out (halfed)
		 * according to the zoomflag
		 * True : Zoom out
		 * False : Zoom In
		 */
		public function ZoomImage(zoomFlag:Boolean):void
		{
			if(zoomFlag)
				this.Zoom(0.5*Width, 0.5*Height, 2.0);
			else
				this.Zoom(0.5*Width, 0.5*Height, 0.5);
		}

		/**
		 * This function zooms to the given magnification relative to the specified point.
		 */
		public function Zoom(x:Number, y:Number, mag:Number ):void
		{
			var newXPageSize:Number = GetXPageSize() / mag;
			if (newXPageSize > MAX_PAGE_SIZE||newXPageSize<1)
				return;
			var extraBit:Number = ( GetXPageSize() - newXPageSize ) * Number(x) /Number(Width);
			var newXPos:Number = XPosition + extraBit;
			var newYPageSize:Number = GetYPageSize() / mag;
			extraBit = ( GetYPageSize() - newYPageSize ) * Number(y) / Number(Height);
			var newYPos:Number = YPosition - extraBit;
			XPosition = newXPos;
			YPosition = newYPos;
			if(newXPageSize>2)
				XPageSize = newXPageSize;
			if(newYPageSize>2)
				YPageSize = newYPageSize;
			IsZoomed = true;
			//Alert.show("inside zoom"+xPosition.toString(),"gg");	
		}
		
		public function getXPageSize():Number
		{
			return XPageSize;
		}
		
		public function setXPageSize(l:Number ):void
		{
			XPageSize = l;
		}
		
		public function getYPageSize():Number
		{
			if(Width != 0)
				return (Number(XPageSize) * Number(Height) /Number(Width));
			else
				return XPageSize;
		}
		
		public function setYPageSize(l:Number ):void
		{
			if(Height == 0)
				XPageSize = l;
			else
				XPageSize = Width /Height * l;
		}
		
		/**
		 * This function transforms the distance from our image coordinate distance into
		 * real world distance.
		 */
		public function iTransformDist(  dx:Number, dy:Number ):Array
		{
			var temp:Array=new Array();
			var dx2:Number = Number(dx)/Number(Width) * getXPageSize();
			var dy2:Number = -Number(dy)/Number(Height) * getYPageSize();
		 	temp.push(dx2);
			temp.push(dy2);
			return (temp);
		}
	   	
		/**
		 * This function transforms the distance in real world to distance in 
		 * our image coordinates.
		 */
		public function transformDist( dx:Number, dy:Number ):Array
		{
			var dx2:Number = Number( dx / getXPageSize() *Width );
			var dy2:Number = Number( -dy / getYPageSize() * Height );
			var temp:Array=new Array(dx2, dy2);
			return (temp);
		}
		
		/**
		 * Function to clamp the page sizes to MAX_PAGE_SIZE.
		 */
		public function makeNearestValidWindow( xPos:Number, yPos:Number, xPageSize:Number, yPageSize:Number ):Array
		{	   	
			if (xPageSize > MAX_PAGE_SIZE)
				xPageSize = MAX_PAGE_SIZE;
			if (yPageSize > MAX_PAGE_SIZE)
				yPageSize = MAX_PAGE_SIZE;
			var temparr:Array=new Array(xPos,yPos,xPageSize);
			return (temparr);
		}
	}
}