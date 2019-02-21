

class Debug(PyObjectPlus):
	"""This class records each step of the entire PostProcessing chain
	into a render target, allowing you to see every single step. 

	This kind of debugging is required as the individual phases in the
	PostProcessing chain may (and should) be re-using render targets. """
	
	renderTarget = None  	#PyRenderTarget.  
	
	"""renderTarget: 
	The Debug object requires a render target to do its work. With this
	attribute you can provide the Debug object with a render target of
	whatever size and format you like. 
	Type: PyRenderTarget. """
	
	def phaseUV( self, effect, phase, nEffects, nPhases ): 
		"""This method returns the texture coordinates for the given 
		effect/phase pair, packed into a Vector4 as ( bl.x, bl.y, tr.x, tr.y ), or l, b, r, t ) 
		Parameters: effect  index of the effect within the PostProcessing chain  
		phase  index of the phase within the effect  
		nEffects  number of effects  
		nPhases  number of phases within the desired effect  
		
		
		Returns: Vector4 packed uv coordinates of the phase  """
		return


class Effect(PyObjectPlus):
	"""This class allows the post-processing manager to work with
	a generic list of effects; it also ensures that all effects are python objects. """
	
	bypass = None   	#Vector4Provider.  
	name   = None		#String.  
	phases = None 		#List.
	
	"""bypass: 
	Type: Vector4Provider.  
	
	
	name: 
	Type: String.  
	
	
	phases: 
	This attribute represents the individual phases within an effect. It is
	managed as a list, so you can add, remove and re-order the phases within
	the effect. 
	Type: List."""
	
	return
	
class PyCopyBackBuffer(PyObjectPlus):
	"""This class derives from PostProcessing::Phase, and provides a way
	to get a copy of the back buffer in a render target. 

	It uses a draw cookie to know when the back buffer has been updated. If backBufferCopy.draw is
	called but the render target already has the latest copy of the render target then it optimises
	out the draw call. Note this optimisation only works if you reuse the same PyCopyBackBuffer
	phase instance. """
	
	name  = None 		#String.  
	renderTarget = None  	#PyRenderTarget.  
	
	"""name: 
	Arbitrary name identifying this phase. 
	Type: String.  
	
	
	renderTarget: 
	The render target in which a copy of the back buffer will be placed. Internally
	the DirectX function StretchRect is used, so please refer to the DirectX documentation
	for information on restrictions regarding pairs of back buffer formats and render target
	formats. 
	Type: PyRenderTarget."""
	
	return

class PyFilterQuad(PyObjectPlus):
	"""A PyFilterQuad can be used by a PyPhase object, and provides the geometry for the
	phase. The FilterQuad performs n-taps on the source material's textures, in groups
	of 4. If you specify more than 4 taps, then the filter quad will draw using n/4 passes. For
	these operations, you should make sure the material you use is additive, so the results
	can accumulate. """
	
	samples  = None 		#List of Vector3s.  
	
	return

class PyPhase(PyObjectPlus):
	"""This class derives from PostProcessing::Phase, and provides a generic phase designed
	to be configured entirely via python. A phase is a single step within a PostProcessing
	Effect. It generally performs a single draw of a filter quad using a material into
	a render target. """
	
	clearRenderTarget = None   		#Boolean.  
	filterQuad = None  			#PyFilterQuad.  
	material   = None			#PyMaterial.  
	name  = None 				#String.  
	renderTarget = None  			#PyRenderTarget.
	
	"""clearRenderTarget: 
	Type: Boolean.  
	
	
	filterQuad: 
	Type: PyFilterQuad.  
	
	
	material: 
	Type: PyMaterial.  
	
	
	name: 
	Type: String.  
	
	
	renderTarget: 
	Type: PyRenderTarget.  
	
	"""	
	
	return

class PyPointSpriteTransferMesh(PyObjectPlus):
	"""A PyPointSpriteTransferMesh can be used by a PyPhase object, and
	provides the geometry for the phase. In this case, the geometry is
	one point sprite for every pixel on the screen. The PointSpriteTransferMesh
	performs n-taps on the source material's textures, in groups of 4. If you specify
	more than 4 taps, then the filter quad will draw using n/4 passes. For these 
	operations, you should make sure the material you use is additive, so the results can accumulate. """
	
	return

class PyTransferQuad (PyObjectPlus):
	"""A PyTransferQuad can be used by a PyPhase object, and provides the geometry
	for the phase. The TransferQuad fills out 4 texture coordinates for its vertices, but
	each uv coordinate is not offset at all. """
	return

class PyVisualTransferMesh(PyObjectPlus):
	"""This class implements a FilterQuad that just draws a visual file. """
	
	resourceID = None  	#String.  
		
	return


def CopyBackBuffer( ): 
	"""Returns: A new PostProcessing PyCopyBackBuffer object.  """


def Debug( ): 
	"""Returns: A new PostProcessing Debug object.  """


def Effect( ): 
	"""Returns: A new PostProcessing Effect object.  """


def FilterQuad( ): 
	"""Returns: A new PostProcessing PyFilterQuad object."""
def Phase( ): 
	"""Returns: A new PostProcessing PyPhase object.  """


def Phase( ): 
	"""Factory function to create and return a PostProcessing PyPhase object. 
	Returns: A new PostProcessing PyPhase object.  """


def PointSpriteTransferMesh( ): 
	"""Returns: A new PostProcessing PyPointSpriteTransferMesh object.  """


def TransferQuad( ): 
	"""Returns: A new PostProcessing PyTransferQuad object."""

def VisualTransferMesh( ): 
	"""Returns: A new PostProcessing PyVisualTransferMesh object.  """


def chain( Any ): 
	"""This function sets or gets the entire post-processing chain of effects. 
	Parameters: Any  sequence of PyEffects, or None.

        Returns: The global PyEffects list, if no arguments were passed in, or None."""


def debug( A ): 
	"""This function sets or gets the debug object for the PostProcessing chain. 
	Parameters: A  PostProcessing.Debug object, or None.  
	
	
	Returns: A PostProcessing.Debug object, or None."""

def load( DataSection ): 
	"""This function load a post-processing chain. Note that the resulting chain is not automatically set as the global chain. 
	Parameters: DataSection   
	
	
	Returns: PyEffects list."""  


def profile( N ): 
	"""This function profiles the post-processing chain, returning the average GPU time incurred. 
	Parameters: N  (optional)number of samples to take  


	Returns: float average GPU time, in seconds, taken to process the chain. """ 


def save( DataSection ): 
	"""This function saves the post-processing chain. 
	Parameters: DataSection   
	
	
	Returns: None
	"""

