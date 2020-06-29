\ProvidesClass{kis}

\RequirePackage[a4paper]{geometry}
\DeclareOption{draft}{ \geometry{showframe}
					\PassOptionsToClass{\CurrentOption}{article}}
\ProcessOptions\relax

\LoadClass[12pt]{article}

\RequirePackage[doublespacing]{setspace}
\RequirePackage[final]{graphicx}
\RequirePackage[ngerman]{babel}
\RequirePackage{tabularx}
\RequirePackage{hhline}
\RequirePackage{titlesec}
\RequirePackage[format=hang, font=small, labelfont=bf, figurename=Abb., tablename=Tab.]{caption}
\RequirePackage[style=numeric, sorting=none, backend=bibtex, maxbibnames=99]{biblatex}
\DeclareFieldFormat[article,inbook,incollection,inproceedings,patent,thesis,unpublished]{citetitle}{\emph{#1}}
\DeclareFieldFormat[article,inbook,incollection,inproceedings,patent,thesis,unpublished]{title}{\glqq #1\grqq}
\RequirePackage{xcolor}
\RequirePackage{needspace}
\RequirePackage{textcomp}
\RequirePackage{float}
\RequirePackage{wrapfig}
\RequirePackage{amsmath}
\RequirePackage[acronym]{glossaries}
\RequirePackage{parnotes}
\RequirePackage[final]{listings}
\RequirePackage[hidelinks]{hyperref}
\RequirePackage{fancyhdr}
\RequirePackage{microtype}

\makenoidxglossaries

\geometry{left = 30mm,
		 right = 25mm,
		 top = 25mm,
		 bottom = 25mm
}

\pagestyle{fancy}
\fancyhf{}
\fancyhead[L]{KIS -- \@title}
\fancyhead[R]{\today}
\renewcommand{\headrulewidth}{0.5pt}
\fancyfoot[C]{\thepage}
\renewcommand{\footrulewidth}{0pt}

\setlength{\parindent}{0pt}
\setlength{\parskip}{8pt}

\clubpenalty10000
\widowpenalty10000

\renewcommand{\maketitle}{
\begin{titlepage}
\centering
Technische Universität Ilmenau

Fakultät für Automatik und Automatisierung
\vfill

{\Large\bf Komplexe Informationstechnische Systeme

\vspace{5mm}

\LARGE\@title}

\vspace{5mm}

Sommersemester 2020

\vfill

\begin{tabularx}{\textwidth}{XcXcX}
	& Jakob Hampel && Sophie Altenburg &\\
	& Ingenieurinformatik && Informatik &\\
	& 57208 && 57605&\\
\end{tabularx}

\vfill
\today
\end{titlepage}
}

\newenvironment{blockquote}[1]
	{\def\@bibkey{#1}\relax\begin{quote}\it\small}
	{\end{quote} \vspace{-7mm}\hfill\citeauthor*{\@bibkey} in \citetitle{\@bibkey} \cite{\@bibkey}\bigskip}

\renewcommand{\thesection}{\arabic{section}}

\titleformat{\section}[hang]{\clearpage\needspace{25mm} \Large\bf}{Aufgabe \thesection)}{0.5em}{}
\titleformat{\subsection}[hang]{\needspace{10mm} \large\bf}{\thesubsection}{0.5em}{}
\titleformat{\subsubsection}[hang]{\needspace{10mm} \bf}{\thesubsubsection}{0.5em}{}

\renewcommand\paragraph{%
  \@startsection{paragraph}
    {4}
    {\z@}
    {3.25ex \@plus1ex \@minus.2ex}
    {-1em}
    {\normalfont\normalsize\bfseries\add@dot}%
}
\newcommand{\add@dot}[1]{#1.}