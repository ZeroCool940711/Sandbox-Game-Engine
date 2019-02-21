package
{
	import flash.display.BitmapData;
	import flash.display.Graphics;
	import flash.geom.Matrix;
	import mx.core.BitmapAsset;
	import mx.skins.RectangularBorder;

	public class ConfigurationBackground extends RectangularBorder
	{
		private var tile:BitmapData;
		[Embed(source="/images/conf-bg.gif")]
		public var imgCls:Class;

		public function ConfigurationBackground():void
		{
			var background:BitmapAsset 	= BitmapAsset(new imgCls());
			this.tile 					=  background.bitmapData;
		}
		
		override protected function updateDisplayList(unscaledWidth:Number, unscaledHeight:Number):void 
		{
			super.updateDisplayList(unscaledWidth, unscaledHeight);
			var transform: Matrix = new Matrix();

			// Finally, copy the resulting bitmap into our own graphic context.
			graphics.clear();
			graphics.beginBitmapFill(this.tile, transform, true);
			graphics.drawRect(0, 0, unscaledWidth, unscaledHeight);
		}
	}
}