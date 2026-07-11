#region File Description
//-----------------------------------------------------------------------------
// Game1.cs
//
// Minimal XNA 4.0 CullMode reproducer for the CNA culling-compatibility
// investigation (see cna_graphics/docs/xna_culling_compatibility_audit.md
// and this sample's own README.md for the full context).
//
// Renders two flat-colored triangles of OPPOSITE winding under all four
// combinations relevant to RasterizerState.CullMode, so the result can be
// directly compared against the equivalent CNA reproducer
// (examples/rasterizerstate_cullmode_camera_test.cpp in cna_graphics),
// which uses the exact same vertex data, camera, and projection.
//
// No content pipeline, no textures, no models, no lighting -- deliberately
// matching Phase 1 of the audit's own "minimal reproducer" requirement.
//-----------------------------------------------------------------------------
#endregion

using System;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace CullModeTest
{
    /// <summary>
    /// Draws the same two test triangles four times, in four screen quadrants,
    /// each quadrant using a different RasterizerState.CullMode. Compare this
    /// screenshot against the equivalent CNA screenshot to establish authoritative
    /// XNA 4.0 CullMode behavior.
    /// </summary>
    public class Game1 : Microsoft.Xna.Framework.Game
    {
        GraphicsDeviceManager graphics;
        BasicEffect effect;

        // Two triangles with OPPOSITE winding, hand-verified via the shoelace formula
        // (see the CNA reproducer's own header comment for the derivation):
        //   triCW:  bottom-left -> bottom-right -> top, REVERSED order (v0,v2,v1)
        //           -> negative signed area in NDC-space X/Y.
        //   triCCW: bottom-left -> bottom-right -> top, natural order
        //           -> positive signed area in NDC-space X/Y.
        // Placed via the SAME camera's own right/up basis vectors as the CNA test
        // (Matrix.CreateLookAt's own vectorB/vectorC), not raw world axes, so both
        // triangles land predictably on screen regardless of camera orientation.
        VertexPositionColor[] triCW;
        VertexPositionColor[] triCCW;

        Matrix view;
        Matrix projection;

        static readonly Color kBackground = Color.DarkSlateGray;
        static readonly Color kRed = Color.Red;
        static readonly Color kGreen = Color.Lime;

        public Game1()
        {
            graphics = new GraphicsDeviceManager(this);
            graphics.PreferredBackBufferWidth = 800;
            graphics.PreferredBackBufferHeight = 600;
            Content.RootDirectory = "Content";
        }

        protected override void Initialize()
        {
            base.Initialize();

            // SimpleAnimation's own exact camera (SimpleAnimation.cs's Draw()):
            //   eye=(1000,500,0), target=(0,150,0), up=(0,1,0),
            //   FOV=MathHelper.PiOver4, near=10, far=10000.
            Vector3 eye = new Vector3(1000f, 500f, 0f);
            Vector3 target = new Vector3(0f, 150f, 0f);
            Vector3 up = new Vector3(0f, 1f, 0f);
            view = Matrix.CreateLookAt(eye, target, up);

            float aspect = GraphicsDevice.Viewport.AspectRatio;
            projection = Matrix.CreatePerspectiveFieldOfView(MathHelper.PiOver4, aspect, 10f, 10000f);

            Vector3 camBack = Vector3.Normalize(eye - target);
            Vector3 camRight = Vector3.Normalize(Vector3.Cross(up, camBack));
            Vector3 camUp = Vector3.Cross(camBack, camRight);

            Vector3 centerA = target + camRight * -80f;
            Vector3 centerB = target + camRight * 80f;

            triCW = MakeTriBasis(centerA, camRight, camUp, 40f, 60f, kRed, reversed: true);
            triCCW = MakeTriBasis(centerB, camRight, camUp, 40f, 60f, kGreen, reversed: false);

            effect = new BasicEffect(GraphicsDevice);
            effect.VertexColorEnabled = true;
            effect.View = view;
            effect.Projection = projection;
            effect.World = Matrix.Identity;
        }

        static VertexPositionColor[] MakeTriBasis(Vector3 center, Vector3 right, Vector3 up,
            float halfW, float halfH, Color color, bool reversed)
        {
            Vector3 bl = center - right * halfW - up * halfH;
            Vector3 br = center + right * halfW - up * halfH;
            Vector3 top = center + up * halfH;

            if (!reversed)
            {
                return new[]
                {
                    new VertexPositionColor(bl, color),
                    new VertexPositionColor(br, color),
                    new VertexPositionColor(top, color),
                };
            }
            else
            {
                // Reversed order -> negated signed area (opposite winding of the un-reversed case).
                return new[]
                {
                    new VertexPositionColor(bl, color),
                    new VertexPositionColor(top, color),
                    new VertexPositionColor(br, color),
                };
            }
        }

        void DrawBoth(CullMode mode)
        {
            RasterizerState rs = new RasterizerState();
            rs.CullMode = mode;
            GraphicsDevice.RasterizerState = rs;

            effect.CurrentTechnique.Passes[0].Apply();
            GraphicsDevice.DrawUserPrimitives(PrimitiveType.TriangleList, triCW, 0, 1);
            GraphicsDevice.DrawUserPrimitives(PrimitiveType.TriangleList, triCCW, 0, 1);
        }

        void DrawBothDefaultState()
        {
            // Deliberately do NOT set GraphicsDevice.RasterizerState at all -- exercises
            // whatever the real default RasterizerState is, exactly like SimpleAnimation's
            // own Tank.Draw(), which never sets RasterizerState either.
            effect.CurrentTechnique.Passes[0].Apply();
            GraphicsDevice.DrawUserPrimitives(PrimitiveType.TriangleList, triCW, 0, 1);
            GraphicsDevice.DrawUserPrimitives(PrimitiveType.TriangleList, triCCW, 0, 1);
        }

        protected override void Draw(GameTime gameTime)
        {
            GraphicsDevice.Clear(kBackground);

            int w = GraphicsDevice.Viewport.Width;
            int h = GraphicsDevice.Viewport.Height;
            int halfW = w / 2;
            int halfH = h / 2;

            Viewport full = GraphicsDevice.Viewport;

            // Top-left: CullMode.None -- both triangles should be visible (red AND green).
            GraphicsDevice.Viewport = new Viewport(0, 0, halfW, halfH);
            DrawBoth(CullMode.None);

            // Top-right: CullMode.CullClockwiseFace.
            GraphicsDevice.Viewport = new Viewport(halfW, 0, halfW, halfH);
            DrawBoth(CullMode.CullClockwiseFace);

            // Bottom-left: CullMode.CullCounterClockwiseFace (XNA's documented default value).
            GraphicsDevice.Viewport = new Viewport(0, halfH, halfW, halfH);
            DrawBoth(CullMode.CullCounterClockwiseFace);

            // Bottom-right: whatever RasterizerState.CullMode actually IS by default, without
            // this test ever setting RasterizerState at all -- must match the bottom-left
            // quadrant exactly if XNA's real default is genuinely CullCounterClockwiseFace.
            GraphicsDevice.Viewport = new Viewport(halfW, halfH, halfW, halfH);
            DrawBothDefaultState();

            GraphicsDevice.Viewport = full;

            base.Draw(gameTime);

            // Quadrant legend (also printed once to the Visual Studio Output window via
            // System.Diagnostics.Debug, in case reading small on-screen text from a VM
            // screenshot is inconvenient):
            //   top-left     = CullMode.None                     (expect: RED left tri visible, GREEN right tri visible)
            //   top-right    = CullMode.CullClockwiseFace
            //   bottom-left  = CullMode.CullCounterClockwiseFace  (XNA's documented default)
            //   bottom-right = default RasterizerState (never explicitly set)
            // Compare bottom-left and bottom-right: they MUST look identical if the real
            // default RasterizerState truly is CullCounterClockwiseFace.
            if (!loggedOnce)
            {
                loggedOnce = true;
                System.Diagnostics.Debug.WriteLine("CullModeTest quadrants: " +
                    "top-left=None, top-right=CullClockwiseFace, " +
                    "bottom-left=CullCounterClockwiseFace(default), bottom-right=UnsetDefaultState. " +
                    "Each quadrant draws a RED triangle (CW winding, left half) and a GREEN " +
                    "triangle (CCW winding, right half). Take a screenshot and compare against " +
                    "the equivalent CNA screenshot (examples/rasterizerstate_cullmode_camera_test.cpp).");
            }
        }

        bool loggedOnce = false;
    }
}
