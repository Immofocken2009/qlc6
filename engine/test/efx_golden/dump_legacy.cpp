// One-off generator: dumps the LEGACY EFX::calculatePoint math to a CSV so the
// JSON presets can be checked for parity. Only buildable against the old engine.
#include <QCoreApplication>
#include <QTextStream>
#include <QFile>
#include <cmath>
#include "doc.h"
#include "efx.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Doc doc(&app);
    QFile out(argc > 1 ? argv[1] : "efx_legacy_golden.csv");
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
        return 1;
    QTextStream ts(&out);
    ts.setRealNumberPrecision(9);
    ts << "algo,xfreq,yfreq,xphase,yphase,width,height,rotation,xoffset,yoffset,dir,startoffset,iter,x,y\n";

    struct Liss { int xf, yf, xp, yp; };
    const Liss lissVariants[] = { {1,1,0,0}, {2,3,90,45}, {0,0,0,0}, {0,2,30,0}, {3,0,0,60} };
    const int offsets[] = { 0, 45, 180 };
    struct Xf { int w, h, rot, xo, yo; };
    const Xf xforms[] = { {127,127,0,127,127}, {80,60,30,100,140} };
    const int N = 32;

    for (int a = EFX::Circle; a <= EFX::Lissajous; a++)
    {
        const bool liss = (a == EFX::Lissajous);
        const int nvar = liss ? 5 : 1;
        for (int v = 0; v < nvar; v++)
        {
            EFX efx(&doc);
            efx.setAlgorithm(EFX::Algorithm(a));
            Liss l = lissVariants[v];
            if (liss)
            {
                efx.setXFrequency(l.xf); efx.setYFrequency(l.yf);
                efx.setXPhase(l.xp);     efx.setYPhase(l.yp);
            }
            for (const Xf &xf : xforms)
            {
            efx.setWidth(xf.w); efx.setHeight(xf.h); efx.setRotation(xf.rot);
            efx.setXOffset(xf.xo); efx.setYOffset(xf.yo);
            for (int d = 0; d < 2; d++)
            {
                Function::Direction dir = d ? Function::Backward : Function::Forward;
                for (int so : offsets)
                    for (int i = 0; i < N; i++)
                    {
                        float it = float(M_PI * 2.0 * i / N);
                        float x = 0, y = 0;
                        efx.calculatePoint(dir, so, it, &x, &y);
                        ts << QString::fromLatin1(EFX::algorithmToString(EFX::Algorithm(a)).toLatin1())
                           << ',' << (liss ? l.xf : efx.xFrequency()) << ',' << (liss ? l.yf : efx.yFrequency())
                           << ',' << (liss ? l.xp : efx.xPhase()) << ',' << (liss ? l.yp : efx.yPhase())
                           << ',' << xf.w << ',' << xf.h << ',' << xf.rot << ',' << xf.xo << ',' << xf.yo
                           << ',' << d << ',' << so << ',' << it << ',' << x << ',' << y << '\n';
                    }
            }
            }
        }
    }
    return 0;
}
