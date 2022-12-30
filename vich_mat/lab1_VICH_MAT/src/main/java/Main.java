import java.util.Vector;

public class Main {



    public static void main(String[] args) {
        Double a = 1.0; //1, -10, 6.223, -1
        Double b = -2.98;
        Double c = -9.08;
        Double d = -5.1;
        Double epsilon = 0.001;

        Equation equation = new Equation(a,b,c,d,epsilon);
        Vector<Double> rootsEquation= equation.run();
        System.out.println("answ" + rootsEquation);
//        Double D = 1.0;

    }

}
