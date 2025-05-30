{
    // Create VOF solver
    geometricVofExt::SimPLIC::solveVofEqu plicVofSolver(alpha1, phi, U);


    // Reconstruct and write interface at the initial time (t = 0)
    plicVofSolver.reconstruct();
    runTime.functionObjects().execute();


    Info<< "\nStarting time loop\n" << endl;

    while (runTime.run())
    {
        #include "readDyMControls.H"
        #include "CourantNo.H"
        #include "alphaCourantNo.H"
        #include "setDeltaT.H"

        ++runTime;

        Info<< "Time = " << runTime.timeName() << nl << endl;

        // Used for AMR
        if (isA<dynamicRefineFvMesh>(mesh))
        {
            plicVofSolver.reconstruct();

            mesh.update();

            if (mesh.changing())
            {
                plicVofSolver.mapAlphaField();
            }
        }

        #include "updateU.H"

        plicVofSolver.reconstruct();    //Interface reconstruction

        #include "alphaSuSp.H"
        plicVofSolver.advect(Sp, Su);   //Interface advection

        Info<< nl << "Phase-1 volume = "
            << gSum(alpha1.primitiveField() * mesh.V())
            << nl << endl;

        Info<< "Times consumed by steps:" << nl
            << "  Alpha mapping   = " << plicVofSolver.alphaMappingTime() << nl
            << "  Reconstruction  = " << plicVofSolver.reconstructionTime() << nl
            << "  Advection       = " << plicVofSolver.advectionTime() << nl
            << endl;

        runTime.write();

        runTime.printExecutionTime(Info);
    }
}
