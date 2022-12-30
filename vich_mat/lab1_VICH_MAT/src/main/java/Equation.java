import java.util.Vector;

public class Equation {
    Double a;
    Double b;
    Double c;
    Double d;
    Double epsilon;

    Equation(Double a,Double b,Double c,Double d, Double epsilon){
        this.a = a;
        this.b = b;
        this.c = c;
        this.d = d;
        this.epsilon = epsilon;
    }

    public Vector<Double> run(){
        Vector<Double> roots = new Vector<Double>();
        Vector<Double> roots2Derivative= findRoots2(a*3,b*2,c, epsilon);
        System.out.println(roots2Derivative);
        if(roots2Derivative.size() == 1){
            Vector<Double> lineBorder = findLineSegment(roots2Derivative.get(0));
            double root = bisection(lineBorder);
            roots.add(root);
        }
        else{
            boolean leftOnUp = checkAnswerInPoint(roots2Derivative.get(0)) > 0;
            boolean rightOnDown = checkAnswerInPoint(roots2Derivative.get(1)) < 0;
            if(leftOnUp){
                Vector<Double> lineBorder = findLineSegment(roots2Derivative.get(0));
                double root = bisection(lineBorder);
                roots.add(root);
            }
            if(leftOnUp && rightOnDown){
                double root = bisection(roots2Derivative);
                roots.add(root);
            }
            if(rightOnDown) {
                Vector<Double> lineBorder = findLineSegment(roots2Derivative.get(1));
                double root = bisection(lineBorder);
                roots.add(root);
            }
        }
        return roots;
    }

    private Vector<Double> findRoots2(Double a, Double b, Double c, Double epsilon) {
        Double discriminant = b * b - 4 * a * c;
        Vector<Double> roots = new Vector<>();
        if(discriminant < epsilon){
            Double root = b /(2 * a);
            roots.add(root);
            System.out.println("Equation have 1 root : " + root);
        }else{ // D > eps
            Double root1 = (0 - b - Math.sqrt(discriminant))/ (2 * a);
            Double root2 = (0 - b + Math.sqrt(discriminant))/ (2 * a);
            roots.add(root1);
            roots.add(root2);
            //Оцениваем расстояние до оси x в точках эксремума. Если всё ок, то пользуемся бисекцией 3 - n раз (n - приближение
            // к оси x в точках экстремума)
//find 1 root (змейка выше или ниже)
        }
        return roots;
    }

    private double bisection(Vector<Double> lineSegment) {
        Vector<Double> ownLineSegment = lineSegment;
        double leftBorder = ownLineSegment.get(0);
        double rightBorder = ownLineSegment.get(1);
        double funcGrow = 0 - Math.signum(checkAnswerInPoint(leftBorder));
//        System.out.println("Знак функции в левой точке = " + funcGrow);
        double middlePoint = (leftBorder + rightBorder) / 2;
        double answerInPoint = checkAnswerInPoint(middlePoint);
        int iteration = 1;
        while(answerInPoint > epsilon || answerInPoint < 0 - epsilon){
            if(checkAnswerInPoint(middlePoint) * funcGrow < 0){
                leftBorder = middlePoint;
            }
            else{
                rightBorder = middlePoint;
            }
            middlePoint = (leftBorder + rightBorder) / 2;
            answerInPoint = checkAnswerInPoint(middlePoint);
            iteration++;
        }
        //----Доп step
//        if(checkAnswerInPoint(middlePoint) * funcGrow < 0){
//            leftBorder = middlePoint;
//        }
//        else{
//            rightBorder = middlePoint;
//        }
//        middlePoint = (leftBorder + rightBorder) / 2;
//        answerInPoint = checkAnswerInPoint(middlePoint);
        System.out.println("Итераций : " + iteration);
        //
        return middlePoint;
    }

    private Vector<Double> findLineSegment(double startPoint/*, boolean positive*/) {
        double addStep;
        double sgnStartPoint;

        if(checkAnswerInPoint(startPoint) > 0){
            addStep = -1024.0;
            sgnStartPoint = 1.0;
        }else{
            addStep = 1024.0;
            sgnStartPoint = -1.0;
        }

        double firstPoint = startPoint;
        while(checkAnswerInPoint(firstPoint + addStep) * sgnStartPoint > 0){
            firstPoint +=addStep;
        }
        double secondPoint = firstPoint + addStep;
        Vector<Double>  answer = new Vector<Double>();
        answer.add(firstPoint);
        answer.add(secondPoint);
        return answer;
    }

    private double checkAnswerInPoint(double point) {
        double answer = a * point * point * point +
                b * point * point +
                c * point +
                d;
        return answer;
    }

}
