
using UnityEngine;
using VRC.SDK3.Components;
using VRC.SDKBase;
using VRC.Udon;
using UdonSharp;

/// <summary>
/// A Basic example class that demonstrates how to toggle a list of object on and off when someone interacts with the UdonBehaviour
/// This toggle only works locally
/// </summary>
[AddComponentMenu("Udon Sharp/Utilities/Mat And Interact Toggle")]
[UdonBehaviourSyncMode(BehaviourSyncMode.NoVariableSync)]
public class MatAndInteractToggle : UdonSharpBehaviour 
{
	[Tooltip("List of objects to toggle on and off")]
	public GameObject toggleObject;
	public Material[] matFlip;
	Quaternion originalMirrorR;
	Vector3 originalMirrorP;
	bool bSetOriginal = false;

	void Start()
	{
		foreach (Material m in matFlip)
		{
			m.SetFloat( "_Flip", toggleObject.activeSelf?1.0f:0.0f );
		}
	}

	public override void Interact()
	{
		if( !bSetOriginal )
		{
			originalMirrorR = toggleObject.transform.rotation;
			originalMirrorP = toggleObject.transform.position;
			bSetOriginal = true;
		}
		toggleObject.transform.rotation = originalMirrorR;
		toggleObject.transform.position = originalMirrorP;
		bool bLastSet = false;
		toggleObject.SetActive(bLastSet = !toggleObject.activeSelf);
		
		foreach (Material m in matFlip)
		{
			m.SetFloat( "_Flip", bLastSet?1.0f:0.0f );
		}
	}
	
	void Update()
	{
		Vector4 rotation = new Vector4( toggleObject.transform.rotation.x, toggleObject.transform.rotation.y,toggleObject.transform.rotation.z, toggleObject.transform.rotation.w );
		Vector3 scale = toggleObject.transform.localScale;
		Vector3 pos = toggleObject.transform.position;
		foreach (Material m in matFlip)
		{
			m.SetVector( "_MirrorPlace", pos );
			m.SetVector( "_MirrorScale", scale );
			m.SetVector( "_MirrorRotation", rotation );
		}
	}
}
