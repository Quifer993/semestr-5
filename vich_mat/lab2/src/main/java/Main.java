public class Main {

    private static void printA(double[][] a, int n) {
        for (int i = 0; i < n;i++){
            for (int j = 0; j < n; j++){
                if(j - i == -1 || j - i == 1){
                    System.out.print(-a[i][j] + " " + "\t");
                }
                else{
                    System.out.print(a[i][j] + " " + "\t");
                }
            }
            System.out.println();
        }
    }

    private static void initialized(double[][] A,
                                    double[] f,
                                    double[] aVector,
                                    double[] bVector,
                                    double[] cVector,
                                    int n,
                                    double e, double y) {
        for(int i = 0; i < aVector.length; i++){
            aVector[i] = 1;
        }

        for(int i = 0; i < bVector.length; i++){
            bVector[i] = 1;
        }
        //условие c
        for(int i = 0; i < cVector.length; i++){
//            cVector[i] = 2;
//            cVector[i] = 2;
            cVector[i] = 2 * i + y;
        }
        //условие f
        for(int i = 0; i < n;i++){
//            f[i] = 2;
//            f[i] = 2 + e;
            f[i] = 2 * (i + 1) + y;;
        }

        for (int i = 0; i < n;i++){
            for (int j = 0; j < n; j++){
                if(j - i == -1){
                    A[i][j] = aVector[j];
                }
                else if(j - i == 1){
                    A[i][j] = bVector[i];
                }
                else if(j - i == 0){
                    A[i][j] = cVector[i];
                }
                else{
                    A[i][j] = 0;
                }
            }
        }

    }

    private static void printVector(double[] vector, int n) {
        for(int i = 0 ; i < n ; i++){
            System.out.print(vector[i] + "|");
        }
        System.out.println();
    }

    private static double[] solution(double[][] A,
                                     double[] f,
                                     double[] a,
                                     double[] b,
                                     double[] c,
                                     int n) {
        double x[] = new double[n];
        double alpha[] = new double[n];
        double betta[] = new double[n];

        alpha[0] = b[0]/c[0];
        betta[0] = f[0]/c[0];



        for(int i = 1 ; i < n; i++){
            if(i < n - 1) {
                alpha[i] = b[i] / (c[i] - a[i - 1] * alpha[i - 1]);
            }
            betta[i] = (f[i] + a[i - 1]*betta[i - 1]) / (c[i] - a[i - 1] * alpha[i - 1]);
        }
//        printVector(alpha, n);
//        printVector(betta, n);

        x[n-1] = betta[n-1];
        for(int i = n - 2 ; i >= 0; i--){
            x[i] = alpha[i] * x[i + 1] + betta[i];
        }

        return x;
    }

    public static void main(String[] args) {
        int N = 7;
        double e = 0;
        double y = 4.5;

        double[][] A = new double[N][N];
        double[] f = new double[N];
        double[] aVector = new double[N - 1];
        double[] bVector = new double[N - 1];
        double[] cVector = new double[N];

        initialized(A, f, aVector, bVector, cVector, N, e, y);
        System.out.println("A : ");
        printA(A, N);
        System.out.println("f: ");
        printVector(f, N);
        System.out.println("x : ");
        printVector(solution(A, f, aVector, bVector, cVector, N), N);

    }

}
