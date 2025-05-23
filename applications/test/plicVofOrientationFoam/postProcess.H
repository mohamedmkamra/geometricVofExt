/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016 OpenFOAM Foundation
    Copyright (C) 2018 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Global
    postProcess

Description
    Execute application functionObjects to post-process existing results.

    If the "dict" argument is specified the functionObjectList is constructed
    from that dictionary otherwise the functionObjectList is constructed from
    the "functions" sub-dictionary of "system/controlDict"

    Multiple time-steps may be processed and the standard utility time
    controls are provided.

\*---------------------------------------------------------------------------*/

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifndef CREATE_TIME
    #define CREATE_TIME createTime.H
#endif

#ifndef CREATE_MESH
    #define CREATE_MESH createDynamicFvMesh.H
#endif

#ifndef CREATE_FIELDS
    #define CREATE_FIELDS createFields.H
#endif

#ifndef CREATE_CONTROL
    #define CREATE_CONTROL createControl.H
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#define INCLUDE_FILE(X) INCLUDE_FILE2(X)
#define INCLUDE_FILE2(X) #X

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

Foam::argList::addBoolOption
(
    argList::postProcessOptionName,
    "Execute functionObjects only"
);

if (argList::postProcess(argc, argv))
{
    Foam::timeSelector::addOptions();
    #include "addRegionOption.H"
    #include "addFunctionObjectOptions.H"

    // Set functionObject post-processing mode
    functionObject::postProcess = true;

    #include "setRootCase.H"

    if (args.found("list"))
    {
        functionObjectList::list();
        return 0;
    }

    #include INCLUDE_FILE(CREATE_TIME)
    Foam::instantList timeDirs = Foam::timeSelector::select0(runTime, args);
    #include INCLUDE_FILE(CREATE_MESH)

    #ifndef NO_CONTROL
    #include INCLUDE_FILE(CREATE_CONTROL)
    #endif

    // Externally stored dictionary for functionObjectList
    // if not constructed from runTime
    dictionary functionsDict;

    HashSet<wordRe> selectedFields;

    // Construct functionObjectList
    autoPtr<functionObjectList> functionsPtr
    (
        functionObjectList::New(args, runTime, functionsDict, selectedFields)
    );

    forAll(timeDirs, timei)
    {
        runTime.setTime(timeDirs[timei], timei);

        Info<< "Time = " << runTime.timeName() << endl;

        switch (mesh.readUpdate())
        {
            case polyMesh::POINTS_MOVED:
            {
                functionsPtr->movePoints(mesh);
                break;
            }
            case polyMesh::TOPO_CHANGE:
            case polyMesh::TOPO_PATCH_CHANGE:
            {
                mapPolyMesh mpm(mesh);
                functionsPtr->updateMesh(mpm);
                break;
            }
            case polyMesh::UNCHANGED:
            {
                // No additional work
                break;
            }
            default:
            {
                FatalErrorIn(args.executable())
                    << "Unhandled enumeration"
                    << abort(FatalError);
            }
        }

        FatalIOError.throwing(true);

        try
        {
            #include INCLUDE_FILE(CREATE_FIELDS)

            #ifdef CREATE_FIELDS_2
            #include INCLUDE_FILE(CREATE_FIELDS_2)
            #endif

            #ifdef CREATE_FIELDS_3
            #include INCLUDE_FILE(CREATE_FIELDS_3)
            #endif

            geometricVofExt::SimPLIC::reconstruction reconstructor
            (
                alpha1,
                mesh.solverDict(alpha1.name())
            );

            reconstructor.reconstruct();

            functionsPtr->execute();

            // Execute the functionObject 'end()' function for the last time
            if (timei == timeDirs.size()-1)
            {
                functionsPtr->end();
            }

            // Report to output (avoid overwriting values from simulation)
            profiling::print(Info);
        }
        catch (const Foam::IOerror& err)
        {
            Warning<< err << endl;
        }

        Info<< endl;
    }

    Info<< "End\n" << endl;

    return 0;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#undef INCLUDE_FILE
#undef INCLUDE_FILE2

#undef CREATE_MESH
#undef CREATE_FIELDS
#undef CREATE_CONTROL

// ************************************************************************* //
