tmp<volScalarField> rAU;

if (correctPhi)
{
    rAU = new volScalarField
    (
        IOobject
        (
            "rAU",
            runTime.timeName(),
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("rAU", dimTime/dimDensity, 1)
    );

    CorrectPhi
    (
        U,
        phi,
        p_rgh,
        surfaceScalarField("rAUf", fvc::interpolate(rAU())),
        fvc::div(phi),
        pimple
    );

    #include "continuityErrs.H"
}
else
{
    CorrectPhi
    (
        U,
        phi,
        p_rgh,
        dimensionedScalar("rAUf", dimTime/rho.dimensions(), 1),
        fvc::div(phi),
        pimple
    );

    #include "continuityErrs.H"
}
