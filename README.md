# PCS Playground

This project is a small Unreal Engine host project for the [PCS (Point Cloud
Sequence)](https://github.com/yatagai-mm/pcs) plugin. PCS loads and renders a
sequence of XYZRGB point-cloud frames stored as binary PLY files.

## Requirements

- Unreal Engine 5.8
- Git
- A binary little-endian PLY sequence containing `x`, `y`, `z`, `red`, `green`,
  and `blue` vertex properties

## Get the Project

Clone the project and initialize the PCS submodule:

```sh
git clone --recurse-submodules https://github.com/yatagai-mm/pcs-playground.git
cd pcs-playground
```

If the project has already been cloned without submodules, run:

```sh
git submodule update --init --recursive
```

The plugin is checked out at `Plugins/PCS`. Open `PCSHost.uproject` in Unreal
Editor. PCS is enabled in the project descriptor and should appear in the
Plugins window as **PCS**.

## Use PCS in the Editor

1. Create an Actor, or open an existing Actor Blueprint.
2. Add a **Point Cloud Sequence** component.
3. Set **Sequence Directory** to the folder containing the PLY frames.
4. Set **Frame File Name Regex** so capture group 1 contains the frame number.
5. Set **Frame Rate**, **Point Size**, **Playback Rate**, and **Loop** as needed.
6. Leave **Auto Play** enabled to start playback when the Actor begins play.

The default file-name pattern is:

```text
^frame_(\d+)\.ply$
```

It matches files such as `frame_000001.ply` and `frame_000002.ply`. For a
different naming scheme, use a pattern such as:

```text
^capture_(\d+)\.ply$
```

The component sorts matching files by the integer captured by group 1. After
changing the source directory or regex at runtime, call **Refresh Sequence**
or use `SetSequenceSource` from Blueprint or C++.

## Use PCS from C++

Add `PCS` to the module dependencies in your module's `Build.cs` file:

```csharp
PrivateDependencyModuleNames.Add("PCS");
```

Then create and configure a `UPointCloudSequenceComponent`:

```cpp
#include "PointCloudSequenceComponent.h"

APointCloudActor::APointCloudActor()
{
	PointCloud = CreateDefaultSubobject<UPointCloudSequenceComponent>(TEXT("PointCloud"));
	SetRootComponent(PointCloud);

	PointCloud->SequenceDirectory.Path = TEXT("Content/PointClouds");
	PointCloud->FrameFileNameRegex = TEXT("^frame_(\\d+)\\.ply$");
	PointCloud->FrameRate = 30.0f;
	PointCloud->PointSize = 1.0f;
	PointCloud->bAutoPlay = true;
	PointCloud->bLoop = true;
}
```

The component also exposes `Play`, `Pause`, `Stop`, `SeekFrame`, `SeekTime`,
`SetSequenceDirectory`, `SetSequenceSource`, and `RefreshSequence` to
Blueprints and C++.

## PLY Input Requirements

PCS expects each frame to be a binary little-endian PLY 1.0 file. Each file
must contain a vertex element with scalar `x`, `y`, and `z` position properties
and `red`, `green`, and `blue` color properties. Positions remain in the units
stored in the PLY file, so adjust the component or Actor scale when the source
data uses a different unit system.

The loader reads frames asynchronously. Large sequences may take a moment to
show the first frame, and the sequence directory should be kept available
while the Actor is playing.

## This Sample

The sample Actor in `Source/PCSHost` demonstrates a scene root with an attached
PCS component. The included level is `Content/pcs-host-actor.umap`.
