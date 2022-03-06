
using Texel;
using UdonSharp;
using UnityEngine;
using VRC.SDKBase;
using VRC.Udon;

namespace EmissionUpdate
{
    [AddComponentMenu("Udon Sharp/Video/Emissive Updater")]
    public class EmissiveUpdater2 : UdonSharpBehaviour
    {
        MeshRenderer rendererToUpdate;

        void Start()
        {
            rendererToUpdate = GetComponent<MeshRenderer>();
            RendererExtensions.UpdateGIMaterials(rendererToUpdate);
        }

        private void Update()
        {
			RendererExtensions.UpdateGIMaterials(rendererToUpdate);
        }
    }
}
